#define LED_COUNT 240
#include <Adafruit_NeoPixel.h>
#include "FastLED.h"

#define BRIGHTNESS 100

#define ENCODER_DIR   1
#define ENCODER_OUT_A 6 // CLK
#define ENCODER_OUT_B 7 // DT
#define ENCODER_OUT_C 8 // SW
#define LED_OUT       9

Adafruit_NeoPixel leds(LED_COUNT, LED_OUT, NEO_GRBW + NEO_KHZ800);

#define COIL_REP 20
#define LED_SPACE_MM 17
#define OVERRELAX 1.9
#define N_CELLS  LED_COUNT + 2 * COIL_REP
#define CELLA_I  COIL_REP
#define CELLZ_I  CELLA_I + LED_COUNT - 1

#define _B *256

// physical/math constants
static const float G = 10;
static const float LIFT = 60;
static const float SIN30 = 0.5;
static const float SIN60 = 0.866;

float cell_x[N_CELLS];
float cell_y[N_CELLS];
void cell_n_to_xy(uint16_t n, float *out_x, float *out_y);

float u[N_CELLS];
float v[N_CELLS];
float w[N_CELLS];
float new_u[N_CELLS];
float new_v[N_CELLS];
float new_w[N_CELLS];
float t[N_CELLS];
float new_t[N_CELLS];

uint32_t last_t;
uint32_t last_draw_t;

uint32_t last_heat_change = 0;

void setup() {
  delay(1000);
  randomSeed(analogRead(0));
  random16_set_seed(analogRead(0));

  leds.begin();
  leds.show();
  leds.setBrightness(BRIGHTNESS);

  pinMode(ENCODER_OUT_A, INPUT);
  pinMode(ENCODER_OUT_B, INPUT);
  pinMode(ENCODER_OUT_C, INPUT);

  float cx, cy;
  for (uint16_t i = 0; i < N_CELLS; i++) {
    u[i] = 0.0;
    v[i] = 0.0;
    w[i] = 0.0;
    new_u[i] = 0.0;
    new_v[i] = 0.0;
    new_w[i] = 0.0;
    t[i] = 0.0;
    new_t[i] = 0.0;

    cell_n_to_xy(i, &cx, &cy);
    cell_x[i] = cx;
    cell_y[i] = cy;
  }

  Serial.begin(9600);
}

inline uint16_t cell(int16_t n) {
  if (n < 0) return 0;
  else if (n >= N_CELLS) return N_CELLS - 1;
  else return n;
}

const float nx[6] = { 1.0f,  0.5f, -0.5f, -1.0f, -0.5f,  0.5f };
const float ny[6] = { 0.0f,  0.866025f,  0.866025f,  0.0f, -0.866025f, -0.866025f };
const float row_x_offset = 0.5f;
const float row_y = 0.866025f;
const float max_x = 19%20 + 240/20 * 0.5 + 0.5;

inline float face_vel(uint16_t n, uint8_t k) {
  switch (k) {
    case 0: return u[cell(n)];
    case 1: return v[cell(n)];
    case 2: return w[cell(n)];
    case 3: return -u[cell(n - 1)];
    case 4: return -v[cell(n - COIL_REP)];
    case 5: return -w[cell(n - COIL_REP + 1)];
    default: return 0.0f;
  }
}

uint16_t cell_xy_to_n(float px, float py) {
  uint8_t row = (int)(py / row_y + 0.5f);
  // uint8_t col = (int)(px - row * row_x_offset + 0.5f);
  // return cell(row * COIL_REP + col);
  return cell(row * 19.5 + px);
}

void cell_n_to_xy(uint16_t n, float *out_x, float *out_y) {
  uint8_t crow = n / 20;
  // uint8_t ccol = n % 20;
  // uint8_t ccol = 2 * n % 39;
  uint8_t ccol = fmod(n, 19.5);
  // *out_x = ccol + crow * row_x_offset;
  *out_y = crow * row_y;
  *out_x = ccol;
}

void reconstruct_velocity(uint16_t n, float *out_x, float *out_y) {
  float sum_x = 0.0f, sum_y = 0.0f;
  for (uint8_t f = 0; f < 6; f++) {
    float vel = face_vel(n, f);
    sum_x += vel * nx[f];
    sum_y += vel * ny[f];
  }
  *out_x = sum_x / 3.0f;
  *out_y = sum_y / 3.0f;
}

float sample_face_normal(float px, float py, uint8_t face_dir) {
  static const int8_t offsets[7] = { 0, 1, COIL_REP, COIL_REP - 1, -1, -COIL_REP, -COIL_REP + 1 };
  static const float r = 0.5f;

  uint16_t n = cell_xy_to_n(px, py);

  float total_w = 0.0f, total_v = 0.0f;
  float cx, cy;

  for (uint8_t i = 0; i < 7; i++) {
    uint16_t cn = cell(n + offsets[i]);
    // float crow = cn / 20.0f;
    // uint8_t ccol = cn % 20;
    // float cx = ccol + crow * row_x_offset;
    // float cy = crow * row_y;
    cx = cell_x[cn];
    cy = cell_y[cn];
    // cell_n_to_xy(cn, &cx, &cy);
    float fx = cx + r * nx[face_dir];
    float fy = cy + r * ny[face_dir];
    float dx = px - fx, dy = py - fy;
    while (dx > max_x / 2) dx -= max_x;
    while (dx < -max_x / 2) dx += max_x;
    float dist2 = dx * dx + dy * dy;
    float w_i = 1.0f / (dist2 + 1e-6f);
    total_v += w_i * face_vel(cn, face_dir);
    total_w += w_i;
  }
  return total_v / total_w;
}

float sample_temp(float px, float py) {
  static const int8_t offsets[7] = { 0, 1, COIL_REP, COIL_REP - 1, -1, -COIL_REP, -COIL_REP + 1 };
  static const float r = 0.5f;

  uint16_t n = cell_xy_to_n(px, py);

  float total_w = 0.0f, total_v = 0.0f;
  float cx, cy;

  for (uint8_t i = 0; i < 7; i++) {
    uint16_t cn = cell(n + offsets[i]);
    // float crow = cn / 20.0f;
    // uint8_t ccol = cn % 20;
    // float cx = ccol + crow * row_x_offset;
    // float cy = crow * row_y;
    cx = cell_x[cn];
    cy = cell_y[cn];
    // cell_n_to_xy(cn, &cx, &cy);
    float dx = px - cx, dy = py - cy;
    while (dx > max_x / 2) dx -= max_x;
    while (dx < -max_x / 2) dx += max_x;
    float dist2 = dx * dx + dy * dy;
    float w_i = 1.0f / (dist2 + 1e-6f);
    total_v += w_i * t[cn];
    total_w += w_i;
  }
  return total_v / total_w;
}

// add gravity and lift forces
void integrate(float dt) {
  for (uint16_t i = 0; i < N_CELLS; i++) {
    float fall = -G * dt;
    float target_v = t[i] * LIFT;
    float lift = target_v - (v[i] + w[i]) / 2;
    float u_target;
    if (i > COIL_REP * 4 && i < COIL_REP * 5) {
      u_target = -10.0;// + 40.0 * ((float)i / (float)(N_CELLS));
    } else if (i > COIL_REP * 11 && i < COIL_REP * 12) {
      u_target = 20.0;// + 40.0 * ((float)i / (float)(N_CELLS));
    }
    u[i] += (u_target - u[i]) * 0.75;
    v[i] += (fall + lift) * SIN60;
    w[i] += (fall + lift) * SIN60;
  }
}

void solve_incompressibility(uint8_t nIters) {
  for (uint8_t iter = 0; iter < nIters; iter++) {
    for (uint16_t i = CELLA_I; i <= CELLZ_I; i++) {
      uint8_t su1 = get_s(i - 1);
      uint8_t su2 = get_s(i + 1);
      uint8_t sv1 = get_s(i - COIL_REP);
      uint8_t sv2 = get_s(i + COIL_REP);
      uint8_t sw1 = get_s(i - COIL_REP + 1);
      uint8_t sw2 = get_s(i + COIL_REP - 1);
      uint8_t s = su1 + su2 + sv1 + sv2 + sw1 + sw2;
      if (s == 0) { return; }

      float div = u[i] - u[i - 1] + v[i] - v[i - COIL_REP] + w[i] - w[i - COIL_REP + 1];
      float push = -div / s * OVERRELAX;
      u[i] += push * su2;
      u[i - 1] -= push * su1;
      v[i] += push * sv2;
      v[i - COIL_REP] -= push * sv1;
      w[i] += push * sw2;
      w[i - COIL_REP + 1] -= push * sw1;
    }
  }
}

void extrapolate() {
  if (millis() - last_heat_change > 100) {
    last_heat_change = millis();
    for (uint16_t i = CELLA_I; i < CELLA_I + 2 * COIL_REP; i++) {
      if (random8(3) == 0) {
        t[i] += 1.2;
      }
    }
  }

  for (uint16_t i = 0; i < CELLA_I; i++) {
    u[i] = u[i + COIL_REP];
    v[i] = v[i + COIL_REP];
    w[i] = w[i + COIL_REP];
    t[i] = t[i + COIL_REP];
  }
  for (uint16_t i = CELLZ_I + 1; i < N_CELLS; i++) {
    u[i] = u[i - COIL_REP];
    v[i] = v[i - COIL_REP];
    w[i] = w[i - COIL_REP];
    t[i] = t[i - COIL_REP];
  }
}

inline uint8_t get_s(uint16_t idx) {
  if (idx >= CELLA_I && idx <= CELLZ_I) {
    return 1;
  }
  return 0;
}

void advect_velocity(float dt) {

  static const float r = 0.5f;
  float cx, cy;

  for (uint16_t n = CELLA_I; n <= CELLZ_I; n++) {
    // float row = n / 20.0f;
    // uint8_t col = n % 20;
    // float cx = col + row * row_x_offset;
    // float cy = row * row_y;
    cx = cell_x[n];
    cy = cell_y[n];
    // cell_n_to_xy(cn, &cx, &cy);

    float vx, vy;
    reconstruct_velocity(n, &vx, &vy);

    for (uint8_t k = 0; k < 3; k++) {
      float fx = cx + r * nx[k];
      float fy = cy + r * ny[k];

      float bx = fx - dt * vx;
      float by = fy - dt * vy;

      float advected = sample_face_normal(bx, by, k);

      if (k == 0) {
        new_u[n] = advected;
      } else if (k == 1) {
        new_v[n] = advected;
      } else {
        new_w[n] = advected;
      }
    }
  }
  memcpy(u, new_u, sizeof(new_u));
  memcpy(v, new_v, sizeof(new_v));
  memcpy(w, new_w, sizeof(new_w));
}

void advect_temperature(float dt) {

  static const float r = 0.5f;
  float cx, cy;

  for (uint16_t n = CELLA_I; n <= CELLZ_I; n++) {
    // float row = n / 20.0f;
    // uint8_t col = n % 20;
    // float cx = col + row * row_x_offset;
    // float cy = row * row_y;
    cx = cell_x[n];
    cy = cell_y[n];
    // cell_n_to_xy(cn, &cx, &cy);

    float vx, vy;
    reconstruct_velocity(n, &vx, &vy);

    float bx = cx - dt * vx;
    float by = cy - dt * vy;

    float advected = sample_temp(bx, by);

    new_t[n] = advected;
  }
  memcpy(t, new_t, sizeof(new_t));
}

void cooling() {
  for (uint16_t i = 0; i < N_CELLS; i++) {
    t[i] *= 0.9;
  }
}

void draw() {
  uint32_t color;
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    uint16_t n = i + CELLA_I;
    if (t[n] > 0.2) {
      float f = (t[n] - 0.2) / 0.8;
      color = leds.ColorHSV((40 * f)_B, 255, 255);
    } else {
      color = leds.ColorHSV(0, 0, 0);
    }
    // if (t[n] < 0.2) {
    //   float f = t[n] / 0.2;
    //   color = leds.ColorHSV(0, 0, 128 * f);
    // } else if (t[n] < 0.5) {
    //   float f = (t[n] - 0.2) / 0.3;
    //   color = leds.ColorHSV(0, 255 * f, 128 * f);
    // } else {
    //   float f = (t[n] - 0.5) / 0.5;
    //   color = leds.ColorHSV((40 * f)_B, 255, 255);
    // }
    // color = leds.ColorHSV((0 + t[i + CELLA_I] * 10)_B, 255, 255);
    leds.setPixelColor(i, color);
  }
  leds.show();
}

void loop() {
  uint32_t d_time = millis() - last_t;
  last_t = millis();
  float d_time_f = (float)d_time / 1000.0f;

  integrate(d_time_f);
  solve_incompressibility(3);
  extrapolate();
  advect_velocity(d_time_f);
  advect_temperature(d_time_f);
  cooling();

  // limit the framerate
  if (millis() - last_draw_t > 30) {
    draw();
    last_draw_t = millis();
  }
}

// -q- "ayiwoSerial.print("a: "); Serial.println(a);jk
