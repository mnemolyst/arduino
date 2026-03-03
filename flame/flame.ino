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
static const float G = 0.00981;
static const float LIFT_ACCEL = 0.05;
static const float SIN30 = 0.5;
static const float SIN60 = 0.866;

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

  for (uint16_t i = 0; i < N_CELLS; i++) {
    u[i] = 0.0;
    v[i] = 0.0;
    w[i] = 0.0;
    new_u[i] = 0.0;
    new_v[i] = 0.0;
    new_w[i] = 0.0;
    t[i] = 0.0;
    new_t[i] = 0.0;
  }

  Serial.begin(9600);
}

inline uint16_t cell(uint16_t n) {
  return ((n % N_CELLS) + N_CELLS) % N_CELLS;
}

const float nx[6] = { 1.0f,  0.5f, -0.5f, -1.0f, -0.5f,  0.5f };
const float ny[6] = { 0.0f,  0.866025f,  0.866025f,  0.0f, -0.866025f, -0.866025f };

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
  float col_spacing = 1.0f;
  float row_x_offset = 0.5f;
  float row_y = 0.866025f;

  uint8_t row = (int)(py / row_y + 0.5f);
  uint8_t col = (int)(px - row * row_x_offset + 0.5f);
  uint16_t n = cell(row * COIL_REP + col);

  static const uint8_t offsets[7] = { 0, 1, COIL_REP, COIL_REP - 1, -1, -COIL_REP, -COIL_REP + 1 };
  static const float r = 0.5f;

  float total_w = 0.0f, total_v = 0.0f;
  for (uint8_t i = 0; i < 7; i++) {
    uint16_t cn = cell(n + offsets[i]);
    uint8_t crow = cn / 20;
    uint8_t ccol = cn % 20;
    float cx = ccol + crow * row_x_offset;
    float cy = crow * row_y;
    float fx = cx + r * nx[face_dir];
    float fy = cy + r * ny[face_dir];
    float dx = px - fx, dy = py - fy;
    float dist2 = dx * dx + dy * dy;
    float w_i = 1.0f / (dist2 + 1e-6f);
    total_v += w_i * face_vel(cn, face_dir);
    total_w += w_i;
  }
  return total_v / total_w;
}

float sample_temp(float px, float py) {
  float col_spacing = 1.0f;
  float row_x_offset = 0.5f;
  float row_y = 0.866025f;

  uint8_t row = (int)(py / row_y + 0.5f);
  uint8_t col = (int)(px - row * row_x_offset + 0.5f);
  uint16_t n = cell(row * COIL_REP + col);

  return t[n];
}

// add gravity and lift forces
void integrate(uint32_t dt) {
  for (uint16_t i = 0; i < N_CELLS; i++) {
    float fall = -G * dt;
    float lift = t[i] * LIFT_ACCEL * dt;
    v[i] += (fall + lift) * SIN60;
    w[i] += (fall + lift) * SIN60;
  }
}

inline uint8_t get_s(uint16_t idx) {
  if (idx >= CELLA_I && idx <= CELLZ_I) {
    return 1;
  }
  return 0;
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
      // if (i == CELLA_I + COIL_REP) {
      // Serial.println("BEFORE");
      // Serial.print("u[i]: ");
      // Serial.println(u[i]);
      // Serial.print("u[i-1]: ");
      // Serial.println(u[i-1]);
      // Serial.print("v[i]: ");
      // Serial.println(v[i]);
      // Serial.print("v[i-COIL_REP]: ");
      // Serial.println(v[i-COIL_REP]);
      // Serial.print("w[i]: ");
      // Serial.println(w[i]);
      // Serial.print("w[i-COIL_REP+1]: ");
      // Serial.println(w[i-COIL_REP+1]);
      // Serial.print("s: ");
      // Serial.println(s);
      // Serial.print("div: ");
      // Serial.println(div);
      // Serial.print("push: ");
      // Serial.println(push);
      // }
      u[i] += push * su2;
      u[i - 1] -= push * su1;
      v[i] += push * sv2;
      v[i - COIL_REP] -= push * sv1;
      w[i] += push * sw2;
      w[i - COIL_REP + 1] -= push * sw1;
      // if (i == CELLA_I + COIL_REP) {
      // Serial.println("AFTER");
      // Serial.print("u[i]: ");
      // Serial.println(u[i]);
      // Serial.print("u[i-1]: ");
      // Serial.println(u[i-1]);
      // Serial.print("v[i]: ");
      // Serial.println(v[i]);
      // Serial.print("v[i-COIL_REP]: ");
      // Serial.println(v[i-COIL_REP]);
      // Serial.print("w[i]: ");
      // Serial.println(w[i]);
      // Serial.print("w[i-COIL_REP+1]: ");
      // Serial.println(w[i-COIL_REP+1]);
      // }
    }
  }
}

void extrapolate() {
  for (uint16_t i = 0; i < CELLA_I; i++) {
    new_u[i] = 0.0;
    new_v[i] = 0.0;
    new_w[i] = 0.0;
    new_t[i] = 10.0;
  }
  for (uint16_t i = CELLZ_I + 1; i < N_CELLS; i++) {
    new_u[i] = u[i];
    new_v[i] = v[i];
    new_w[i] = w[i];
  }
}

void advect_velocity(uint32_t dt) {
  float dt_f = (float)dt;

  static const float r = 0.5f;
  static const float row_x_offset = 0.5f;
  static const float row_y = 0.866025f;

  for (uint16_t n = CELLA_I; n <= CELLZ_I; n++) {
    uint8_t row = n / 20;
    uint8_t col = n % 20;
    float cx = col + row * row_x_offset;
    float cy = row * row_y;
    float vx, vy;
    reconstruct_velocity(n, &vx, &vy);

    for (uint8_t k = 0; k < 3; k++) {
      float fx = cx + r * nx[k];
      float fy = cy + r * ny[k];

      float bx = fx - dt_f * vx;
      float by = fy - dt_f * vy;

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

void advect_temperature(uint32_t dt) {
  float dt_f = (float)dt;

  static const float r = 0.5f;
  static const float row_x_offset = 0.5f;
  static const float row_y = 0.866025f;

  for (uint16_t n = CELLA_I; n <= CELLZ_I; n++) {
    uint8_t row = n / 20;
    uint8_t col = n % 20;
    float cx = col + row * row_x_offset;
    float cy = row * row_y;

    float vx, vy;
    reconstruct_velocity(n, &vx, &vy);

    float bx = cx - dt_f * vx;
    float by = cy - dt_f * vy;

    float advected = sample_temp(bx, by);

    new_t[n] = advected;
  }
  memcpy(t, new_t, sizeof(new_t));
}

void draw() {
  uint32_t color;
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    color = leds.ColorHSV((0 + t[i + CELLA_I] * 20)_B, 255, 255);
    leds.setPixelColor(i, color);
  }
  leds.show();
}

void loop() {
  uint32_t d_time = millis() - last_t;
  last_t = millis();

  integrate(d_time);
  solve_incompressibility(2);
  extrapolate();
  advect_velocity(d_time);
  advect_temperature(d_time);
  Serial.println("* ");
  Serial.println(u[0]);
  Serial.println(v[0]);
  Serial.println(w[0]);
  Serial.println(t[0]);
  Serial.println("-");
  Serial.println(u[CELLA_I]);
  Serial.println(v[CELLA_I]);
  Serial.println(w[CELLA_I]);
  Serial.println(t[CELLA_I]);
  Serial.println();

  // limit the framerate
  if (millis() - last_draw_t > 30) {
    draw();
    last_draw_t = millis();
  }
}
