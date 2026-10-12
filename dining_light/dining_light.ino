#define N_LEDS 535

#include "FastLED.h"
#include "presets.h"

#define ENCODER_OUT_A 4 // CLK
#define ENCODER_OUT_B 7 // DT
#define ENCODER_OUT_C 8 // SW
#define LED_OUT 2

#define N_CURSOR 8

typedef enum {
  TOP,
  DIM_V,
  RED_V,
  NEW_V,
  PRESET_V,
  HUE_V,
  SAT_V,
  VAL_V,
  START_V,
  END_V,
  BLEND_V,
  NOISE_V,
} Mode;

typedef enum {
  // TOP
  DIM,
  RED,
  NEW,
  PRESET,
  // NEW
  HUE,
  SAT,
  VAL,
  START,
  END,
  BLEND,
  NOISE,
  PASTE,
} Cursor;

const ColorTemperature color_temperatures[] = {
  Candle,
  Tungsten100W,
  OvercastSky,
};

Mode mode = PRESET_V;
Cursor cursor = PRESET;

uint8_t brightness = 255;
uint8_t temperature = 2; // XXX magic! DirectSunlight

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
uint32_t cursor_blink_t = 0;

uint8_t add_hue;
uint8_t add_sat;
uint8_t add_val;
uint16_t add_start;
uint16_t add_end;
uint8_t add_blend;
uint8_t add_noise;

uint8_t preset;


CRGB leds[N_LEDS];
CRGB _leds[N_LEDS];
uint8_t led_noise[N_LEDS];

void setup() {
  delay(1000);
  random16_set_seed(analogRead(0));

  FastLED.addLeds<NEOPIXEL, LED_OUT>(leds, N_LEDS);
  FastLED.setBrightness(dim8_raw(brightness));
  FastLED.setCorrection(TypicalSMD5050);
  FastLED.setTemperature(color_temperatures[temperature]);
  for (uint16_t i = 0; i < N_LEDS; i++) {
    leds[i] = CRGB::Black;
    _leds[i] = CRGB::Black;
  }
  FastLED.show();

  pinMode(ENCODER_OUT_A, INPUT);
  pinMode(ENCODER_OUT_B, INPUT);
  pinMode(ENCODER_OUT_C, INPUT);

  // load_preset();
  // refresh();
  // draw_new();
  FastLED.show();

  Serial.begin(9600);
}

void loop() {
  state_a = digitalRead(ENCODER_OUT_A);
  state_b = digitalRead(ENCODER_OUT_B);
  state_c = digitalRead(ENCODER_OUT_C);

  //
  // CLICK
  //
  if (state_c != oldstate_c) {
    last_debounce_t = millis();
    oldstate_c = state_c;
  }
  if (state_c == 0 && (millis() - last_debounce_t) >= debounce_delay) {
    if (!clk) {
      clk = true;
      switch (mode) {
        case TOP:
          switch (cursor) {
            case DIM:
              mode = DIM_V;
              break;
            case RED:
              mode = RED_V;
              break;
            case NEW:
              mode = NEW_V;
              cursor = HUE;
              reset_new();
              break;
            case PRESET:
              mode = PRESET_V;
              load_preset();
              break;
          }
          break;
        case DIM_V:
          mode = TOP;
          cursor = DIM;
          break;
        case RED_V:
          mode = TOP;
          cursor = RED;
          break;
        case NEW_V:
          switch (cursor) {
            case HUE:
              mode = HUE_V;
              break;
            case SAT:
              mode = SAT_V;
              break;
            case VAL:
              mode = VAL_V;
              break;
            case START:
              mode = START_V;
              break;
            case END:
              mode = END_V;
              break;
            case BLEND:
              mode = BLEND_V;
              break;
            case NOISE:
              mode = NOISE_V;
              break;
            case PASTE:
              copy_back();
              mode = TOP;
              cursor = DIM;
              break;
          }
          break;
        case PRESET_V:
          mode = NEW_V;
          cursor = VAL;
          break;
        case HUE_V:
        case SAT_V:
        case VAL_V:
        case START_V:
        case END_V:
        case BLEND_V:
        case NOISE_V:
          mode = NEW_V;
          break;
      }
      refresh();
      draw_new();
      draw_cursor();
      FastLED.show();
    } // if (!clk)
  } else {
    clk = false;
  }

  //
  // ROTATE
  //
  if (wheel_dir = read_rotary()) {
    if ((wheel_dir == last_wheel_dir)
      && (millis() - last_wheel_t < 250)) {
      wheel_boost = 20 - 19 * log10(millis() - last_wheel_t - 9) / 2.38;
    } else {
      wheel_boost = 1;
    }
    last_wheel_dir = wheel_dir;
    last_wheel_t = millis();

    switch (mode) {
      case TOP:
        switch (cursor) {
          case DIM:
            if (wheel_dir == 1) {
              cursor = RED;
            } else {
              cursor = PRESET;
            }
            break;
          case RED:
            if (wheel_dir == 1) {
              cursor = NEW;
            } else {
              cursor = DIM;
            }
            break;
          case NEW:
            if (wheel_dir == 1) {
              cursor = PRESET;
            } else {
              cursor = RED;
            }
            break;
          case PRESET:
            if (wheel_dir == 1) {
              cursor = DIM;
            } else {
              cursor = NEW;
            }
            break;
        }
        draw_cursor();
        FastLED.show();
        break;
      case DIM_V:
        wheel_dim(wheel_dir * wheel_boost);
        refresh();
        draw_new();
        FastLED.show();
        break;
      case RED_V:
        wheel_red(wheel_dir);
        refresh();
        draw_new();
        FastLED.show();
        break;
      case NEW_V:
        switch (cursor) {
          case HUE:
            if (wheel_dir == 1) {
              cursor = SAT;
            } else {
              cursor = PASTE;
            }
            break;
          case SAT:
            if (wheel_dir == 1) {
              cursor = VAL;
            } else {
              cursor = HUE;
            }
            break;
          case VAL:
            if (wheel_dir == 1) {
              cursor = START;
            } else {
              cursor = SAT;
            }
            break;
          case START:
            if (wheel_dir == 1) {
              cursor = END;
            } else {
              cursor = VAL;
            }
            break;
          case END:
            if (wheel_dir == 1) {
              cursor = BLEND;
            } else {
              cursor = START;
            }
            break;
          case BLEND:
            if (wheel_dir == 1) {
              cursor = NOISE;
            } else {
              cursor = END;
            }
            break;
          case NOISE:
            if (wheel_dir == 1) {
              cursor = PASTE;
            } else {
              cursor = BLEND;
            }
            break;
          case PASTE:
            if (wheel_dir == 1) {
              cursor = HUE;
            } else {
              cursor = NOISE;
            }
            break;
        }
        draw_cursor();
        FastLED.show();
        break;
      case PRESET_V:
        preset = wheel_clamp(preset, 6, wheel_dir);
        refresh();
        load_preset();
        draw_new();
        FastLED.show();
        break;
      case HUE_V:
        add_hue = wheel_8_cycle(add_hue, wheel_dir * wheel_boost);
        refresh();
        draw_new();
        FastLED.show();
        break;
      case SAT_V:
        add_sat = wheel_8_clamp(add_sat, wheel_dir * wheel_boost);
        refresh();
        draw_new();
        FastLED.show();
        break;
      case VAL_V:
        add_val = wheel_8_clamp(add_val, wheel_dir * wheel_boost);
        refresh();
        draw_new();
        FastLED.show();
        break;
      case START_V:
        add_start = wheel_8_pos(add_start, wheel_dir, wheel_boost);
        add_end = max(add_start, add_end);
        refresh();
        draw_new();
        FastLED.show();
        break;
      case END_V:
        add_end = wheel_8_pos(add_end, wheel_dir, wheel_boost);
        add_start = min(add_start, add_end);
        refresh();
        draw_new();
        FastLED.show();
        break;
      case BLEND_V:
        add_blend = wheel_8_clamp(add_blend, wheel_dir * wheel_boost);
        refresh();
        draw_new();
        FastLED.show();
        break;
      case NOISE_V:
        add_noise = wheel_8_clamp(add_noise, wheel_dir * wheel_boost);
        regenerate_noise();
        refresh();
        draw_new();
        FastLED.show();
        break;
    }
  } // if (wheel_dir = read_rotary())
  else if ((mode != TOP && mode != NEW_V) && millis() - cursor_blink_t > 80) {
    cursor_blink_t = millis();
    draw_cursor();
    FastLED.show();
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

void wheel_dim(int8_t amt) {
  int16_t new_v = brightness + amt;
  brightness = clamp_8(new_v);
  FastLED.setBrightness(dim8_raw(brightness));
  FastLED.show();
}

void wheel_red(int8_t dir) {
  int8_t new_v = temperature + dir;
  if (new_v < 0) {
    new_v = 0;
  }
  if (new_v > 2) { // XXX magic!
    new_v = 2;
  }
  temperature = new_v;
  FastLED.setTemperature(color_temperatures[temperature]);
  FastLED.show();
}

uint8_t wheel_clamp(uint8_t v, uint8_t max_v, int8_t dir) {
  int8_t new_v = v + dir;
  if (new_v < 0) {
    return 0;
  }
  if (new_v > max_v) {
    return max_v;
  }
  return new_v;
}

uint8_t wheel_8_clamp(uint8_t v, int8_t amt) {
  int16_t new_v = v + amt;
  return clamp_8(new_v);
}

uint8_t wheel_8_cycle(uint8_t v, int8_t amt) {
  int16_t new_v = v + amt;
  return cycle_8(new_v);
}

uint16_t wheel_8_pos(uint16_t v, int8_t dir, uint8_t boost) {
  int16_t new_v = v + dir * boost;
  return clamp_pos(new_v);
}

uint16_t clamp_pos(int16_t x) {
  if (x < N_CURSOR) {
    return N_CURSOR;
  }
  if (x >= N_LEDS) {
    return N_LEDS - 1;
  }
  return (uint16_t)x;
}

uint8_t clamp_8(int16_t x) {
  if (x < 0) {
    return 0;
  }
  if (x > 255) {
    return 255;
  }
  return (uint8_t)x;
}

uint8_t cycle_8(int16_t x) {
  while (x < 0) {
    x += 256;
  }
  while (x > 255) {
    x -= 256;
  }
  return (uint8_t)x;
}

void reset_new() {
  add_hue = 0;
  add_sat = 255;
  add_val = 255;
  add_start = N_CURSOR;
  add_end = N_LEDS - 1;
  add_blend = 255;
  add_noise = 0;
}

void regenerate_noise() {
  for (uint16_t i = N_CURSOR; i < N_LEDS; i++) {
    led_noise[i] = random8();
  }
}

void refresh() {
  for (uint16_t i = N_CURSOR; i < N_LEDS; i++) {
    leds[i] = _leds[i];
  }
}

void draw_new() {
  uint16_t add_from;
  uint16_t add_to;
  uint8_t noise_val;
  CRGB existing_color;
  CRGB picked_color;
  CRGB noise_color;
  CRGB noised_color;
  CRGB blended_color;
  if (add_start < add_end) {
    add_from = add_start;
    add_to = add_end + 1;
  } else if (add_start > add_end) {
    add_from = add_end;
    add_to = add_start + 1;
  } else {
    add_from = add_start;
    add_to = add_from + 1;
  }
  for (uint16_t i = add_from; i < add_to; i++) {
    noise_val = led_noise[i];
    existing_color = _leds[i];
    picked_color = CHSV(add_hue, add_sat, add_val);
    noise_color = CHSV(noise_val, add_sat, add_val);
    noised_color = blend(picked_color, noise_color, add_noise);
    blended_color = blend(existing_color, noised_color, add_blend);
    leds[i] = blended_color;
  }
}

void draw_cursor() {
  CRGB color;
  if (mode == TOP) {
    color = CRGB::Red;
  } else if (mode == NEW_V) {
    color = CRGB::Blue;
  } else if (mode == DIM_V
    || mode == RED_V
    || mode == PRESET_V)
  {
    color = CHSV(0, 255, beatsin8(60 /* bpm */, 30, 255));
  } else {
    color = CHSV(160, 255, beatsin8(60 /* bpm */, 30, 255));
  }
  for (uint16_t i = 0; i < N_CURSOR; i++) {
    leds[i] = CRGB::Black;
  }
  switch (cursor) {
    case DIM:    leds[0] = color; break;
    case RED:    leds[1] = color; break;
    case NEW:    leds[2] = color; break;
    case PRESET: leds[3] = color; break;
    case HUE:    leds[0] = color; break;
    case SAT:    leds[1] = color; break;
    case VAL:    leds[2] = color; break;
    case START:  leds[3] = color; break;
    case END:    leds[4] = color; break;
    case BLEND:  leds[5] = color; break;
    case NOISE:  leds[6] = color; break;
    case PASTE:  leds[7] = color; break;
  }
}

void copy_back() {
  for (uint16_t i = 0; i < N_LEDS; i++) {
    _leds[i] = leds[i];
  }
}

void load_preset() {
  switch (preset) {
    case 0:
      reset_new();
      add_hue = 0;
      add_sat = 0;
      break;
    case 1:
      reset_new();
      add_hue = 12;
      break;
    case 2:
      reset_new();
      add_hue = 50;
      break;
    case 3:
      reset_new();
      add_hue = 100;
      break;
    case 4:
      reset_new();
      add_hue = 150;
      break;
    case 5:
      reset_new();
      add_hue = 200;
      break;
    case 6:
      reset_new();
      add_hue = 0;
      break;
  }
}
