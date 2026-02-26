#define LED_COUNT 240

#define DEMO

#include <Adafruit_NeoPixel.h>
#include "FastLED.h"

#define BRIGHTNESS 255

#define ENCODER_DIR 1
#define HUE_STEP 2
#define VAL_STEP 4
#define SAT_STEP 4

#define ENCODER_OUT_A 6 // CLK
#define ENCODER_OUT_B 7 // DT
#define ENCODER_OUT_C 8 // SW
#define LED_OUT 9

#define DEBOUNCE_DELAY 50 // 50 msec
#define MODE_TIMEOUT 10000 // 10 sec
#define MODE_DEMO_LEN 1500 // 1.5 sec
#define DEMO_T_STEP 25

enum KNOB_CHG {
  HUE,
  SAT,
  VAL
};

KNOB_CHG knob_chg = HUE;

Adafruit_NeoPixel leds(LED_COUNT, LED_OUT, NEO_GRBW + NEO_KHZ800);

int32_t hue = 12 * 256;
int16_t sat = 255;
int16_t val = 64;

int8_t wheel_dir;
uint32_t idle_start_t = 0;

void knob_to_hue() {
  int16_t i_hue;
  uint16_t delta;
  uint32_t start_t = millis();
  uint16_t csec_done = 0;
  knob_chg = HUE;
#ifdef DEMO
  while ((delta = millis() - start_t) < MODE_DEMO_LEN) {
    if (read_click()) {
      return change_mode();
    }
    if (wheel_dir = read_rotary()) {
      return change_color(wheel_dir);
    }
    if (delta < csec_done + DEMO_T_STEP) {
      continue;
    } else {
      csec_done = (delta / DEMO_T_STEP) * DEMO_T_STEP;
    }
    i_hue = hue + (uint16_t)(65536 * delta / MODE_DEMO_LEN);
    while (i_hue > 65535) {
      i_hue -= 65536;
    }
    fill(i_hue, sat, val);
  }
  fill(hue, sat, val);
#endif
}

void knob_to_sat() {
  int16_t i_sat;
  uint16_t delta;
  uint32_t start_t = millis();
  double start_v = asin((sat - 127.5) / 127.5);
  uint16_t csec_done = 0;
  knob_chg = SAT;
#ifdef DEMO
  while ((delta = millis() - start_t) < MODE_DEMO_LEN) {
    if (read_click()) {
      return change_mode();
    }
    if (wheel_dir = read_rotary()) {
      return change_color(wheel_dir);
    }
    if (delta < csec_done + DEMO_T_STEP) {
      continue;
    } else {
      csec_done = (delta / DEMO_T_STEP) * DEMO_T_STEP;
    }
    i_sat = (sin(start_v + (delta * 2 * M_PI / MODE_DEMO_LEN)) + 1) * 127.5;
    if (i_sat > 255) {
      i_sat = 255;
    }
    if (i_sat < 0) {
      i_sat = 0;
    }
    fill(hue, i_sat, val);
  }
  fill(hue, sat, val);
#endif
}

void knob_to_val() {
  int16_t i_val;
  uint16_t delta;
  uint32_t start_t = millis();
  double start_v = asin((val - 127.5) / 127.5);
  uint16_t csec_done = 0;
  knob_chg = VAL;
#ifdef DEMO
  while ((delta = millis() - start_t) < MODE_DEMO_LEN) {
    if (read_click()) {
      return change_mode();
    }
    if (wheel_dir = read_rotary()) {
      return change_color(wheel_dir);
    }
    if (delta < csec_done + DEMO_T_STEP) {
      continue;
    } else {
      csec_done = (delta / DEMO_T_STEP) * DEMO_T_STEP;
    }
    i_val = (sin(start_v + (delta * 2 * M_PI / MODE_DEMO_LEN)) + 1) * 127.5;
    if (i_val > 255) {
      i_val = 255;
    }
    if (i_val < 0) {
      i_val = 0;
    }
    fill(hue, sat, i_val);
  }
  fill(hue, sat, val);
#endif
}

void change_mode() {
  switch (knob_chg) {
    case HUE:
      knob_to_val();
      break;
    case SAT:
      knob_to_hue();
      break;
    case VAL:
      knob_to_sat();
      break;
    default:
      knob_to_hue();
      break;
  }
}

void change_color(int8_t wheel_dir) {
  switch (knob_chg) {
    case HUE:
      hue += HUE_STEP * 256 * wheel_dir;
      if (hue < 0) {
        hue += 65536;
      }
      if (hue > 65535) {
        hue -= 65536;
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

  fill(hue, sat, val);
}

void fill(uint16_t h, uint8_t s, uint8_t v) {
  leds.fill(leds.ColorHSV(h, s, v), 0, LED_COUNT);
  leds.show();
}

#define SPARK_COUNT 45
#define COIL_REP 19.3f
typedef struct {
  double x;
  double y;
} Vec;

Vec spark_pos[SPARK_COUNT];
Vec spark_vel[SPARK_COUNT];
Vec spark_acc[SPARK_COUNT];
uint32_t last_t;
uint32_t last_draw_t;
uint8_t intensity = 1;
int8_t wind_dir = 1;

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

void setup() {
  delay(1000);
  randomSeed(analogRead(0));
  random16_set_seed(analogRead(0));

  leds.begin();
  leds.show();
  leds.setBrightness(BRIGHTNESS);
  // fill(hue, sat, val);

  pinMode(ENCODER_OUT_A, INPUT);
  pinMode(ENCODER_OUT_B, INPUT);
  pinMode(ENCODER_OUT_C, INPUT);

  Serial.begin(9600);
  
  for (uint8_t i = 0; i < SPARK_COUNT; i++) {
    spark_pos[i] = {random8(22), 0};
    spark_vel[i] = {0, 0};
    spark_acc[i] = {0, 0};
  }
  draw_sparks();
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

void loop() {
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
  // if (wheel_dir = read_rotary()) {
  //   int8_t new_intensity = intensity + wheel_dir;
  //   if (new_intensity < 1) {
  //     new_intensity = 1;
  //   } else if (new_intensity > 9) {
  //     new_intensity = 9;
  //   }
  //   intensity = new_intensity;
  //   Serial.println(intensity);
  // }

  // SOLID
  if (knob_chg != HUE
      && (millis() - idle_start_t) > MODE_TIMEOUT) {
    knob_to_hue();
  }

  if (read_click()) {
    return change_mode();
  }

  if (wheel_dir = read_rotary()) {
    change_color(wheel_dir);
  }
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
