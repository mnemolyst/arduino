#define LED_COUNT 240

#define DEMO

#include <Adafruit_NeoPixel.h>
#include "FastLED.h"

#define BRIGHTNESS 255

#define ENCODER_DIR 1
#define HUE_STEP 2
#define VAL_STEP 4
#define SAT_STEP 4
#define _B *256

#define ENCODER_OUT_A 6 // CLK
#define ENCODER_OUT_B 7 // DT
#define ENCODER_OUT_C 8 // SW
#define LED_OUT 9

#define DEBOUNCE_DELAY 50 // msec
#define MODE_TIMEOUT 10000 // msec
#define MODE_DEMO_LEN 1000 // msec
#define DEMO_T_STEP 25
#define DOUBLE_CLICK_LIM 250 // msec

Adafruit_NeoPixel leds(LED_COUNT, LED_OUT, NEO_GRBW + NEO_KHZ800);

void fill(uint16_t h, uint8_t s, uint8_t v) {
  leds.fill(leds.ColorHSV(h, s, v));
  leds.show();
}

enum MODE {
  MODE_SOLID,
  MODE_FLAME
};

MODE mode = MODE_SOLID;

// *** START SOLID ***
enum SOLID_KNOB_CHG {
  HUE,
  SAT,
  VAL
};
SOLID_KNOB_CHG solid_knob_chg = VAL;

int32_t hue = 6;
int16_t sat = 224;
int16_t val = 20;

int8_t wheel_dir = 0;
bool did_click = false;
uint32_t last_click_t;
uint32_t idle_start_t = 0;
bool should_escape_demo = false;

void knob_to_hue() {
  int16_t i_hue;
  uint16_t delta;
  uint32_t start_t = millis();
  const uint8_t spread = 10;
  double start_v = 0;
  uint16_t csec_done = 0;
  solid_knob_chg = HUE;
#ifdef DEMO
  should_escape_demo = false;
  while ((delta = millis() - start_t) < MODE_DEMO_LEN) {
    poll_ui();
    if (should_escape_demo) {
      should_escape_demo = false;
      return;
    }
    if (did_click) {
      did_click = false;
      return change_solid_mode();
    }
    if (wheel_dir) {
      wheel_dir = 0;
      return change_color();
    }
    if (delta < csec_done + DEMO_T_STEP) {
      continue;
    } else {
      csec_done = (delta / DEMO_T_STEP) * DEMO_T_STEP;
    }
    // i_hue = hue + (256 * delta / MODE_DEMO_LEN);
    i_hue = hue + spread * sin(start_v + (delta * 2 * M_PI / MODE_DEMO_LEN));
    if (i_hue > 255) {
      i_hue -= 256;
    }
    if (i_hue < 0) {
      i_hue += 256;
    }
    fill((i_hue)_B, sat, val);
  }
  fill((hue)_B, sat, val);
#endif
}

void knob_to_sat() {
  int16_t i_sat;
  uint16_t delta;
  uint32_t start_t = millis();
  const uint8_t spread = 70;
  int16_t low_sat = max(0, sat - spread - max(0, sat - 255 + spread));
  int16_t hi_sat = min(255, sat + spread + max(0, spread - sat));
  int16_t mid_sat = (low_sat + hi_sat) * 0.5;
  double start_v = asin((sat - mid_sat) / (double)spread);
  uint16_t csec_done = 0;
  solid_knob_chg = SAT;
#ifdef DEMO
  should_escape_demo = false;
  while ((delta = millis() - start_t) < MODE_DEMO_LEN) {
    poll_ui();
    if (should_escape_demo) {
      should_escape_demo = false;
      return;
    }
    if (did_click) {
      did_click = false;
      return change_solid_mode();
    }
    if (wheel_dir) {
      wheel_dir = 0;
      return change_color();
    }
    if (delta < csec_done + DEMO_T_STEP) {
      continue;
    } else {
      csec_done = (delta / DEMO_T_STEP) * DEMO_T_STEP;
    }
    i_sat = mid_sat + spread * sin(start_v + (delta * 2 * M_PI / MODE_DEMO_LEN));
    if (i_sat > 255) {
      i_sat = 255;
    }
    if (i_sat < 0) {
      i_sat = 0;
    }
    fill((hue)_B, i_sat, val);
  }
  fill((hue)_B, sat, val);
#endif
}

void knob_to_val() {
  int16_t i_val;
  uint16_t delta;
  uint32_t start_t = millis();
  const uint8_t spread = 40;
  int16_t low_val = max(0, val - spread - max(0, val - 255 + spread));
  int16_t hi_val = min(255, val + spread + max(0, spread - val));
  int16_t mid_val = (low_val + hi_val) * 0.5;
  double start_v = asin((val - mid_val) / (double)spread);
  uint16_t csec_done = 0;
  solid_knob_chg = VAL;
#ifdef DEMO
  should_escape_demo = false;
  while ((delta = millis() - start_t) < MODE_DEMO_LEN) {
    poll_ui();
    if (should_escape_demo) {
      should_escape_demo = false;
      return;
    }
    if (did_click) {
      did_click = false;
      return change_solid_mode();
    }
    if (wheel_dir) {
      wheel_dir = 0;
      return change_color();
    }
    if (delta < csec_done + DEMO_T_STEP) {
      continue;
    } else {
      csec_done = (delta / DEMO_T_STEP) * DEMO_T_STEP;
    }
    i_val = mid_val + spread * sin(start_v + (delta * 2 * M_PI / MODE_DEMO_LEN));
    if (i_val > 255) {
      i_val = 255;
    }
    if (i_val < 0) {
      i_val = 0;
    }
    fill((hue)_B, sat, i_val);
  }
  fill((hue)_B, sat, val);
#endif
}

void change_solid_mode() {
  switch (solid_knob_chg) {
    case HUE:
      knob_to_sat();
      break;
    case SAT:
      knob_to_val();
      break;
    case VAL:
      knob_to_hue();
      break;
    default:
      knob_to_hue();
      break;
  }
}

void change_color() {
  switch (solid_knob_chg) {
    case HUE:
      hue += HUE_STEP * wheel_dir;
      if (hue < 0) {
        hue += 256;
      }
      if (hue > 255) {
        hue -= 256;
      }
      break;
    case SAT:
      sat += SAT_STEP * wheel_dir;
      if (sat < 0) {
        sat = 0;
      }
      if (sat > 255) {
        sat = 255;
      }
      break;
    case VAL:
      val += VAL_STEP * wheel_dir;
      if (val < 0) {
        val = 0;
      }
      if (val > 255) {
        val = 255;
      }
      break;
  }

  fill((hue)_B, sat, val);
}

void loop_solid() {
  if (solid_knob_chg != HUE
      && (millis() - idle_start_t) > MODE_TIMEOUT) {
    knob_to_hue();
  }

  if (did_click) {
    did_click = false;
    return change_solid_mode();
  }

  if (wheel_dir) {
    change_color();
    wheel_dir = 0;
  }
}
// *** END SOLID ***

// *** START FLAME ***
#define COIL_REP 20
#define LED_SPACE_MM 17
#define OVERRELAX 1.9
#define N_CELLS  LED_COUNT + 2 * COIL_REP
#define CELLA_I  COIL_REP
#define CELLZ_I  CELLA_I + LED_COUNT - 1
#define SHIFT 16
#define SHIFT_MASK ((1 << SHIFT) - 1) // 65535
#define MAX_INTENSITY 6

// physical/math constants
static const float G = 10;
static const float LIFT = 30;
static const float SIN30 = 0.5;
static const float SIN60 = 0.866;
static const float _1_2SIN60 = 1.0 / (2 * SIN60);

enum FLAME_KNOB_CHG {
  INTENSITY,
  PALETTE,
  VALUE
};
FLAME_KNOB_CHG flame_knob_chg = INTENSITY;

enum FLAME_PALETTE {
  NORMAL,
  COOL,
  PURPLE,
  PASTEL,
  RAINBOW
};
FLAME_PALETTE flame_palette = NORMAL;

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
uint32_t next_wind_change = 0;
int8_t wind_dir = 1;
uint8_t intensity = 1;
float total_heat;
uint16_t rainbow_offset = 0;

void setup_flame() {
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
}

void blink() {
  fill(0, 0, 0);
  delay(50);
}

void knob_to_intensity() {
  flame_knob_chg = INTENSITY;
  blink();
}

void knob_to_palette() {
  flame_knob_chg = PALETTE;
  blink();
}

void knob_to_value() {
  flame_knob_chg = VALUE;
  blink();
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
const float _1row_y = 1 / row_y;
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
  uint8_t row = (int)(py * _1row_y + 0.5f);
  return cell(row * 19.5 + px);
}

void cell_n_to_xy(uint16_t n, float *out_x, float *out_y) {
  uint8_t crow = n / 20;
  uint8_t ccol = fmod(n, 19.5);
  *out_y = crow * row_y;
  *out_x = ccol;
}

void reconstruct_velocity(uint16_t n, float *out_x, float *out_y) {
  static const float _1_3 = 1.0f / 3.0;
  float sum_x = 0.0f, sum_y = 0.0f;
  for (uint8_t f = 0; f < 6; f++) {
    float vel = face_vel(n, f);
    sum_x += vel * nx[f];
    sum_y += vel * ny[f];
  }
  *out_x = sum_x * _1_3;
  *out_y = sum_y * _1_3;
}

float sample_face_normal(float px, float py, uint8_t face_dir) {
  static const int8_t offsets[7] = { 0, 1, COIL_REP, COIL_REP - 1, -1, -COIL_REP, -COIL_REP + 1 };
  static const float r = 0.5f;

  uint16_t n = cell_xy_to_n(px, py);

  float total_w = 0.0f, total_v = 0.0f;
  float cx, cy;

  for (uint8_t i = 0; i < 7; i++) {
    uint16_t cn = cell(n + offsets[i]);
    cx = cell_x[cn];
    cy = cell_y[cn];
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
    cx = cell_x[cn];
    cy = cell_y[cn];
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

// add gravity, lift, and wind forces
void integrate(float dt) {
  float net_swirl = 0;
  float wind_push;
  if (millis() > next_wind_change) {
    next_wind_change = millis() + 1000 + random16(9000);
    wind_dir = -wind_dir;
  }
  for (uint16_t i = CELLA_I; i <= CELLZ_I; i++) {
    // check the signs on u and v compared to incompressibility!
    net_swirl += u[i] + (v[i] - w[i]) * SIN30;
  }
  wind_push = (net_swirl / LED_COUNT * 0.5) - (3 * intensity * wind_dir);

  for (uint16_t i = CELLA_I; i <= CELLZ_I; i++) {
    float fall = -G * dt;
    float target_v = t[i] * LIFT;
    float lift = target_v - (v[i] + w[i]) * _1_2SIN60;
    u[i] += -wind_push;
    v[i] += (fall + lift) * SIN60 - wind_push;
    w[i] += (fall + lift) * SIN60 + wind_push;
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
      div = u[i] - u[i - 1] + v[i] - v[i - COIL_REP] + w[i] - w[i - COIL_REP + 1];
    }
  }
}

void heating() {
  for (uint16_t i = CELLA_I; i < CELLA_I + COIL_REP - 1; i++) {
    if (random8(6) == 0) {
      t[i] += 0.5f + intensity * 0.4f;
    }
  }
  for (uint16_t i = CELLA_I + COIL_REP + 1; i < CELLA_I + 2 * COIL_REP - 2; i++) {
    if (random8(6) == 0) {
      t[i] += 0.5f + intensity * 0.4f;
    }
  }
}

void extrapolate() {
  for (uint16_t i = 0; i < CELLA_I; i++) {
    u[i] = 0;
    v[i] = 0;
    w[i] = 0;
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
    cx = cell_x[n];
    cy = cell_y[n];

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
    cx = cell_x[n];
    cy = cell_y[n];

    float vx, vy;
    reconstruct_velocity(n, &vx, &vy);

    float bx = cx - dt * vx;
    float by = cy - dt * vy;

    float advected = sample_temp(bx, by);

    new_t[n] = advected;
  }
  memcpy(t, new_t, sizeof(new_t));
}

void cooling(float dt) {
  total_heat = 0;
  float high_frac = pow(0.05, dt);
  for (uint16_t i = CELLA_I; i <= CELLZ_I; i++) {
    total_heat += t[i];
    t[i] *= high_frac;
  }
}

void draw_flame() {
  uint32_t color;
  uint8_t offset;
  uint8_t spread;
  switch (flame_palette) {
    case NORMAL:
      offset = 0;
      spread = 36;
      break;
    case COOL:
      offset = 100;
      spread = 100;
      break;
    case PURPLE:
      offset = 200;
      spread = 55;
      break;
    case PASTEL:
      offset = 0;
      spread = 255;
      break;
    case RAINBOW:
      offset = rainbow_offset;
      spread = 50;
      break;
  }
  for (uint16_t i = 0; i < COIL_REP; i++) {
    if (flame_palette == PASTEL) {
      leds.setPixelColor(i, leds.ColorHSV((offset + (intensity - 1) * spread / MAX_INTENSITY)_B, 192, val));
    } else {
      leds.setPixelColor(i, leds.ColorHSV((offset + (intensity - 1) * spread / MAX_INTENSITY)_B, 255, val));
    }
  }
  for (uint16_t i = COIL_REP; i < LED_COUNT; i++) {
    uint16_t n = i + CELLA_I;
    // DEBUG direction
    // color = leds.ColorHSV((0)_B, 255, 10 * (u[n] + v[n] * SIN30 - w[n] * SIN30));

    float temp = t[n];
    if (temp > 3.8) {
      color = leds.ColorHSV((offset + spread)_B, 255 - (temp - 2.0) * 50, val);
    } else if (temp > 0.2) {
      if (flame_palette == PASTEL) {
        color = leds.ColorHSV((offset + spread / 3.6 * (temp - 0.2))_B, 192, val);
      } else {
        color = leds.ColorHSV((offset + spread / 3.6 * (temp - 0.2))_B, 255, val);
      }
    } else {
      if (flame_palette == PASTEL) {
        color = leds.ColorHSV(0, 0, 0);
      } else {
        color = leds.ColorHSV((offset)_B, 255, 100 * temp * val / 255);
      }
    }
    leds.setPixelColor(i, color);
  }
  leds.show();
}

void change_flame_mode() {
  switch (flame_knob_chg) {
    case INTENSITY:
      knob_to_palette();
      break;
    case PALETTE:
      knob_to_value();
      break;
    case VALUE:
      knob_to_intensity();
      break;
    default:
      knob_to_intensity();
      break;
  }
}

void loop_flame() {
  uint32_t d_time = millis() - last_t;
  last_t = millis();
  float d_time_f = (float)d_time / 1000.0f;
  int8_t new_intensity;

  if (flame_knob_chg != INTENSITY
      && (millis() - idle_start_t) > MODE_TIMEOUT) {
    knob_to_intensity();
  }

  if (wheel_dir) {
    switch (flame_knob_chg) {
      case INTENSITY:
        new_intensity = intensity + wheel_dir;
        switch (wheel_dir) {
          case -1:
            if (new_intensity < 1) {
              new_intensity = 1;
            }
            break;
          case 1:
            if (new_intensity > MAX_INTENSITY) {
              new_intensity = MAX_INTENSITY;
            }
            break;
        }
        intensity = new_intensity;
        Serial.println(intensity);
        break;
      case PALETTE:
        switch (flame_palette) {
          case NORMAL:
            flame_palette = COOL;
            break;
          case COOL:
            flame_palette = PURPLE;
            break;
          case PURPLE:
            flame_palette = PASTEL;
            break;
          case PASTEL:
            flame_palette = RAINBOW;
            break;
          case RAINBOW:
            flame_palette = NORMAL;
            break;
        }
        break;
      case VALUE:
        val += 4 * VAL_STEP * wheel_dir;
        if (val < 0) {
          val = 0;
        }
        if (val > 255) {
          val = 255;
        }
        break;
    }
    wheel_dir = 0;
  }

  if (did_click) {
    did_click = false;
    change_flame_mode();
  }

  integrate(d_time_f);
  poll_ui();

  solve_incompressibility(3);
  poll_ui();

  heating();
  extrapolate();
  poll_ui();

  advect_velocity(d_time_f);
  poll_ui();

  advect_temperature(d_time_f);
  poll_ui();

  cooling(d_time_f);
  poll_ui();

  // limit the framerate
  if (millis() - last_draw_t > 30) {
    if (should_escape_demo) {
      should_escape_demo = false;
      return;
    }

    draw_flame();
    last_draw_t = millis();
  }

  if (flame_palette == RAINBOW) {
    rainbow_offset += d_time / 10;
    while (rainbow_offset > 255) {
      rainbow_offset -= 256;
    }
  }
}
// *** END FLAME ***

// *** START SPARKS ***
#define SPARK_COUNT 45
typedef struct {
  double x;
  double y;
} Vec;

Vec spark_pos[SPARK_COUNT];
Vec spark_vel[SPARK_COUNT];
Vec spark_acc[SPARK_COUNT];

void setup_sparks() {
  for (uint8_t i = 0; i < SPARK_COUNT; i++) {
    spark_pos[i] = {random8(22), 0};
    spark_vel[i] = {0, 0};
    spark_acc[i] = {0, 0};
  }
}

void do_verlet(uint32_t d_time) {
  for (uint8_t i = 0; i < SPARK_COUNT; i++) {
    Vec new_pos;
    Vec new_vel;
    Vec new_acc;
    int16_t new_idx;
    uint16_t idx = spark_pos[i].x + round(spark_pos[i].y) * COIL_REP;

    // accumulate forces
    uint8_t y_lim;
    if (intensity <= 5) {
      y_lim = 4 * COIL_REP;
    } else {
      y_lim = 6 * COIL_REP;
    }
    if (idx < y_lim) {
      if (random8(5) == 0) {
        new_acc = {wind_dir * 0.01, intensity * random(100) / 75};
      } else {
        new_acc = {0.0, -intensity * 0.2};
      }
    } else {
      if (random8(5) == 0) {
        new_acc = {wind_dir * 0.02, intensity * random(100) / 50};
      } else {
        new_acc = {0.0, -intensity * 0.1};
      }
    }
    // update position
    new_pos.x = spark_pos[i].x + spark_vel[i].x * d_time + spark_acc[i].x * d_time * d_time * 0.5;
    new_pos.y = spark_pos[i].y + spark_vel[i].y * d_time + spark_acc[i].y * d_time * d_time * 0.5;
    new_vel.x = spark_vel[i].x + (spark_acc[i].x + new_acc.x) * d_time * 0.5;
    new_vel.y = spark_vel[i].y + (spark_acc[i].y + new_acc.y) * d_time * 0.5;
    new_idx = new_pos.x + round(new_pos.y) * COIL_REP;
    if (new_idx < 0) {
      spark_vel[i] = { new_vel.x, -new_vel.y * 0.8 };
      spark_acc[i] = new_acc;
    } else if (new_idx >= LED_COUNT) { // flyaway; respawn at bottom
      spark_pos[i] = {random8(22), 0};
      spark_vel[i] = {0, 0};
      spark_acc[i] = {0, 0};
    } else {
      spark_pos[i] = new_pos;
      spark_vel[i] = new_vel;
      spark_acc[i] = new_acc;
    }
  }
}

void draw_sparks() {
  leds.clear();

  leds.fill(leds.ColorHSV(0 * 256, 255, 255), 0, COIL_REP);
  uint16_t spark_count = 5 + intensity * 5;
  for (uint8_t i = 0; i < spark_count; i++) {
    uint16_t idx = spark_pos[i].x + round(spark_pos[i].y) * COIL_REP;
    uint32_t color;
    if (idx < (2 * COIL_REP)) {
      color = leds.ColorHSV(0 * 256, 255, 255);
    } else if (idx < (6 * COIL_REP)) {
      color = leds.ColorHSV(random8(4, 10) * 256, 255, 255);
    } else {
      color = leds.ColorHSV(14 * 256, 255, 255);
    }
    leds.setPixelColor(idx, color);
    if (idx > 6 * COIL_REP) {
      if (i % 2 == 0) {
        leds.setPixelColor(idx + floor(COIL_REP), color);
      } else {
        leds.setPixelColor(idx + ceil(COIL_REP), color);
      }
    } else {
      leds.setPixelColor(idx + floor(COIL_REP), color);
      leds.setPixelColor(idx + ceil(COIL_REP), color);
    }
    leds.setPixelColor(idx + ceil(2 * COIL_REP), color);
  }

  leds.show();
}
// *** END SPARKS ***

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

  Serial.begin(9600);
  
  // setup_sparks();
  setup_flame();

  fill((hue)_B, sat, val);
}

void poll_ui() {
  int8_t this_wheel_dir = read_rotary();
  bool this_did_click = read_click();
  if (!wheel_dir)
    wheel_dir = this_wheel_dir;
  if (!did_click) {
    did_click = this_did_click;

    if (did_click) {
      if (millis() - last_click_t < DOUBLE_CLICK_LIM) {
        should_escape_demo = true;
        did_click = false;
        rotate_mode();
      }
      last_click_t = millis();
    }
  }
}

void rotate_mode() {
  if (mode == MODE_SOLID) {
    mode = MODE_FLAME;
    flame_knob_chg = INTENSITY;
  } else if (mode == MODE_FLAME) {
    mode = MODE_SOLID;
    solid_knob_chg = HUE;
    fill((hue)_B, sat, val);
  }
}

void loop() {
  poll_ui();

  switch (mode) {
    case MODE_SOLID:
      loop_solid();
      break;
    case MODE_FLAME:
      loop_flame();
      break;
  }

  // SPARKS
  // uint32_t d_time = millis() - last_t;
  // do_verlet(d_time);
  // last_t = millis();
  // if (millis() - last_draw_t >= 30) {
  //   draw_sparks();
  //   last_draw_t = millis();
  //   if (random8(20) == 0) {
  //     wind_dir = -wind_dir;
  //   }
  // }
}

bool read_click() {
  static uint8_t state_c = digitalRead(ENCODER_OUT_C);
  static uint8_t oldstate_c = 0;
  static uint32_t last_debounce_t;
  static bool clk = false;

  state_c = digitalRead(ENCODER_OUT_C);
  if (state_c != oldstate_c) {
    last_debounce_t = millis();
    oldstate_c = state_c;
  }
  if (state_c == 0 && (millis() - last_debounce_t) >= DEBOUNCE_DELAY) {
    if (!clk) {
      clk = true;
      idle_start_t = millis();
      return true;
    }
  } else {
    clk = false;
  }
  return false;
}

// Copyright John Main - best-microcontroller-projects.com
int8_t read_rotary() {
  static uint16_t rot_store = 0;
  static int8_t rot_enc_table[] = {0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0};
  static uint8_t prev_next_code = 0;

  prev_next_code <<= 2;
  if (digitalRead(ENCODER_OUT_A)) {
    prev_next_code |= 0x01;
  }
  if (digitalRead(ENCODER_OUT_B)) {
    prev_next_code |= 0x02;
  }
  prev_next_code &= 0x0f;
  if (rot_enc_table[prev_next_code]) {
    rot_store <<= 4;
    rot_store |= prev_next_code;
    if ((rot_store & 0xff) == 0x2b) {
      idle_start_t = millis();
      return -1 * ENCODER_DIR;
    }
    if ((rot_store & 0xff) == 0x17) {
      idle_start_t = millis();
      return 1 * ENCODER_DIR;
    }
  }
  return 0;
}
