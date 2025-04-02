#define N_LEDS 535

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

CRGB leds[N_LEDS];

typedef struct Mode {
  int i;
  struct Mode* prev;
  struct Mode* next;
} Mode;

Mode mode1 = {0};
Mode mode2 = {1};

void setup() {
  mode1.next = &mode2;
  mode2.prev = &mode1;

  delay(1000);
  random16_set_seed(analogRead(0));

  FastLED.addLeds<NEOPIXEL, LED_OUT>(leds, N_LEDS);
  FastLED.setBrightness(dim8_raw(brightness));
  FastLED.setCorrection(TypicalSMD5050);
  for (uint16_t i = 0; i < N_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();

  pinMode(ENCODER_OUT_A, INPUT);
  pinMode(ENCODER_OUT_B, INPUT);
  pinMode(ENCODER_OUT_C, INPUT);

  FastLED.show();

  Serial.begin(9600);
}

void loop() {
  state_a = digitalRead(ENCODER_OUT_A);
  state_b = digitalRead(ENCODER_OUT_B);
  state_c = digitalRead(ENCODER_OUT_C);

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
