#define N_LEDS 535

#include <Adafruit_NeoPixel.h>
#include "FastLED.h"

#define ENCODER_OUT_A 4 // CLK
#define ENCODER_OUT_B 7 // DT
#define ENCODER_OUT_C 8 // SW
#define LED_OUT 2

uint8_t brightness = 255;

uint8_t state_a;
uint8_t state_b;
uint8_t state_c = 1;
uint8_t oldstate_c = 0;
bool clk = false;
uint32_t last_debounce_t;
uint8_t debounce_delay = 50;
int8_t wheel_dir;
int8_t last_wheel_dir;
uint32_t last_wheel_t = 0;
uint8_t wheel_boost;

Adafruit_NeoPixel leds(N_LEDS, LED_OUT, NEO_GRBW + NEO_KHZ800);

typedef struct Mode {
  int i;
  struct Mode* prev;
  struct Mode* next;
} Mode;

Mode mode1 = {0};
Mode mode2 = {1};

void setup() {
  delay(1000);
  random16_set_seed(analogRead(0));

  leds.begin();
  leds.show();
  leds.setBrightness(brightness);

  pinMode(ENCODER_OUT_A, INPUT);
  pinMode(ENCODER_OUT_B, INPUT);
  pinMode(ENCODER_OUT_C, INPUT);

  Serial.begin(9600);
}

void loop() {
  state_a = digitalRead(ENCODER_OUT_A);
  state_b = digitalRead(ENCODER_OUT_B);
  state_c = digitalRead(ENCODER_OUT_C);

  for (uint16_t i = 0; i < leds.numPixels(); i++) {
    leds.setPixelColor(i, leds.Color(128, 96, 96));
  }
  leds.show();

  if (state_c != oldstate_c) {
    last_debounce_t = millis();
    oldstate_c = state_c;
  }
  if (state_c == 0 && (millis() - last_debounce_t) >= debounce_delay) {
    if (!clk) {
      clk = true;
    }
  } else {
    clk = false;
  }

  if (wheel_dir = read_rotary()) {
    if ((wheel_dir == last_wheel_dir)
      && (millis() - last_wheel_t < 250)) {
      wheel_boost = 20 - 19 * log10(millis() - last_wheel_t - 9) / 2.38;
    } else {
      wheel_boost = 1;
    }
    last_wheel_dir = wheel_dir;
    last_wheel_t = millis();
  }
}

// Copyright John Main - best-microcontroller-projects.com
uint8_t read_rotary() {
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
      return -1;
    }
    if ((rot_store & 0xff) == 0x17) {
      return 1;
    }
  }
  return 0;
}
