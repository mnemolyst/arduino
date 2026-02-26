#include "FastLED.h"
#include <math.h>

#define BRIGHTNESS 100

// dining room cheap ones = 150
// rig and chandelier ones = 240
#define N_LEDS 240

#define ENCODER_DIR -1

#define ENCODER_OUT_A 6 // CLK
#define ENCODER_OUT_B 7 // DT
#define ENCODER_OUT_C 8 // SW
#define LED_OUT 9

#define STEP 4

#define DEBOUNCE_DELAY 50 // 50 msec
#define MODE_TIMEOUT 10000 // 10 sec
#define MODE_DEMO_LEN 1500 // 1.5 sec

enum KNOB_CHG {
  HUE,
  SAT,
  VAL
};

KNOB_CHG knob_chg = HUE;

CRGB leds[N_LEDS];

int16_t hue = 20;
int16_t sat = 255;
int16_t val = 136;

int8_t wheel_dir;
bool clk = false;
uint32_t idle_start_t = 0;

void setup() {
  delay(1000);
  random16_set_seed(analogRead(0));

  FastLED.addLeds<NEOPIXEL, LED_OUT>(leds, N_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setCorrection(TypicalSMD5050);
  for (uint8_t i = 0; i < N_LEDS; i++) {
    leds[i] = CHSV(hue, sat, val);
  }
  FastLED.show();

  pinMode(ENCODER_OUT_A, INPUT);
  pinMode(ENCODER_OUT_B, INPUT);

  Serial.begin(9600);
}

void knob_to_hue() {
  int16_t i_hue;
  uint16_t delta;
  uint32_t start_t = millis();
  knob_chg = HUE;
  while ((delta = millis() - start_t) < MODE_DEMO_LEN) {
    if (read_click()) {
      return change_mode();
    }
    if (wheel_dir = read_rotary()) {
      return change_color(wheel_dir);
    }
    i_hue = hue + (uint8_t)(255.0 * delta / MODE_DEMO_LEN);
    while (i_hue > 255) { // clamp to <u 255
      i_hue -= 256;
    }
    for (uint8_t i = 0; i < N_LEDS; i++) {
      leds[i] = CHSV(i_hue, sat, val);
    }
    FastLED.show();
  }
}

void knob_to_sat() {
  int16_t i_sat;
  uint16_t delta;
  uint32_t start_t = millis();
  double start_v = asin((sat - 127.5) / 127.5);
  knob_chg = SAT;
  while ((delta = millis() - start_t) < MODE_DEMO_LEN) {
    if (read_click()) {
      return change_mode();
    }
    if (wheel_dir = read_rotary()) {
      return change_color(wheel_dir);
    }
    i_sat = (sin(start_v + (delta * 2 * M_PI / MODE_DEMO_LEN)) + 1) * 127.5;
    if (i_sat > 255) {
      i_sat = 255;
    }
    if (i_sat < 0) {
      i_sat = 0;
    }
    for (uint8_t i = 0; i < N_LEDS; i++) {
      leds[i] = CHSV(hue, i_sat, val);
    }
    FastLED.show();
  }
}

void knob_to_val() {
  int16_t i_val;
  uint16_t delta;
  uint32_t start_t = millis();
  double start_v = asin((val - 127.5) / 127.5);
  knob_chg = VAL;
  while ((delta = millis() - start_t) < MODE_DEMO_LEN) {
    if (read_click()) {
      return change_mode();
    }
    if (wheel_dir = read_rotary()) {
      return change_color(wheel_dir);
    }
    i_val = (sin(start_v + (delta * 2 * M_PI / MODE_DEMO_LEN)) + 1) * 127.5;
    if (i_val > 255) {
      i_val = 255;
    }
    if (i_val < 0) {
      i_val = 0;
    }
    for (uint8_t i = 0; i < N_LEDS; i++) {
      leds[i] = CHSV(hue, sat, i_val);
    }
    FastLED.show();
  }
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
      hue += STEP * wheel_dir;
      if (hue < 0) {
        hue += 256;
      }
      if (hue > 255) {
        hue -= 256;
      }
      break;
    case SAT:
      sat += STEP * wheel_dir;
      if (sat < 0) {
        sat = 0;
      }
      if (sat > 255) {
        sat = 255;
      }
      break;
    case VAL:
      val += STEP * wheel_dir;
      if (val < 0) {
        val = 0;
      }
      if (val > 255) {
        val = 255;
      }
      break;
  }

  for (uint8_t i = 0; i < N_LEDS; i++) {
    leds[i] = CHSV(hue, sat, val);
  }
  FastLED.show();
}

void loop() {
  // return loop_crazy();

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

typedef struct Marquee {
  uint8_t bpm;
  uint8_t len;
  CHSV color1;
  CHSV color2;
} Marquee;

void c1(uint8_t pos, uint32_t t, CHSV *out) {
  if ((pos + scale8(beat8(60, 0), 20)) % 20 > 10) {
  out->hue = 0;
  } else {
  out->hue = 100;
  }
  out->sat = 255;
  out->val = 40;
}

void loop_crazy() {
  static CHSV c;
  uint32_t t = millis();
  for (uint8_t i = 0; i < N_LEDS; i++) {
    c1(i, t, &c);
    leds[i] = c;
  }
  FastLED.show();
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
