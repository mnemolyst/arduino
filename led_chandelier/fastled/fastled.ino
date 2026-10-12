#include "FastLED.h"

#define BRIGHTNESS 25
#define CLUST_HUE_WIDTH 60 // width of hue in each little cluster

#define N_LEDS 480
#define N_CLUST 6

#define N_RING 120

CRGB leds[N_LEDS];

// STRIP 1
const uint16_t clust_leds[N_CLUST][2] = {
  {0, 39},
  {76, 137},
  {177, 219},
  {264, 296},
  {339, 382},
  {421, 443}
};

uint16_t ring_leds[N_RING] = {
46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327, 328, 393, 394, 395, 396, 397, 398, 399, 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 452, 453, 454, 455, 456, 457, 458, 459, 460, 461, 462, 463, 464, 465, 466, 467, 468, 469, 470, 471
};

uint16_t low_hue = 0;
uint16_t high_hue = 90;

uint8_t led_hue_easing_from[N_LEDS];
uint8_t led_hue_easing_to[N_LEDS];
uint16_t led_hue_easing_frac;

uint16_t clust_hue_easing_from[N_CLUST];
uint16_t clust_hue_easing_to[N_CLUST];
uint16_t clust_hue_easing_frac;

uint8_t clust_frac_tic = 0;

int32_t ring_pos = 10;
int32_t prev_ring_pos = 11;
int32_t ring_acc = 0;

void setup() {
  delay(1000);
  random16_set_seed(analogRead(0));

  FastLED.addLeds<NEOPIXEL, 9>(leds, N_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setCorrection(TypicalSMD5050);
  for (uint16_t i = 0; i < N_LEDS; i++) {
    leds[i] = CHSV(0, 0, 0);
  }
  FastLED.show();

  for (uint8_t i = 0; i < N_CLUST; i++) {
    clust_hue_easing_from[i] = get_random_hue();
    clust_hue_easing_to[i] = get_random_hue();

    for (uint16_t j = clust_leds[i][0]; j < clust_leds[i][1]; j++) {
      led_hue_easing_from[j] = random8(CLUST_HUE_WIDTH);
      led_hue_easing_to[j] = random8(CLUST_HUE_WIDTH);
    }
  }
  clust_hue_easing_frac = 0;
  led_hue_easing_frac = 0;
}

void loop() {
  uint8_t hue;
  static uint8_t sin_val = 128;
  bool start_next_clust_ease = false;
  bool start_next_led_ease = false;

  // EASE ALL COLORED LEDS, once every 5 frames
  if (clust_frac_tic++ == 1) {
    clust_frac_tic = 0;
    clust_hue_easing_frac += 1;
    if (clust_hue_easing_frac >= 255) {
      clust_hue_easing_frac = 0;

      start_next_clust_ease = true;
      low_hue = low_hue + 10;
      high_hue = high_hue + 10;
      if (low_hue > 255) {
        low_hue -= 256;
        high_hue -= 256;
      }
    }
  }

  // advance the hue easing
  // pick a new "to" hue if we've finished
  led_hue_easing_frac += 2;
  if (led_hue_easing_frac >= 255) {
    led_hue_easing_frac = 0;

    start_next_led_ease = true;
  }

  for (uint8_t ci = 0; ci < N_CLUST; ci++) {
    if (start_next_clust_ease) {
      clust_hue_easing_from[ci] = clust_hue_easing_to[ci];
      clust_hue_easing_to[ci] = get_random_hue();
    }
    for (uint16_t li = clust_leds[ci][0]; li <= clust_leds[ci][1]; li++) {
      if (start_next_led_ease) {
        led_hue_easing_from[li] = led_hue_easing_to[li];
        led_hue_easing_to[li] = random8(CLUST_HUE_WIDTH);
      }

      hue = get_led_hue(li, ci);
      leds[li] = CHSV(hue, 255, 255);
    }
  }

//  uint16_t ring_hue = (low_hue + 3 * high_hue) / 4;
//  if (ring_hue > 255) {
//    ring_hue -= 256;
//  }
//
//  for (uint8_t i = 0; i < N_RING; i++) {
//    if (i == ring_pos) {
//      leds[ring_leds[i]] = CHSV(ring_hue, 255, 205);
//    } else if (i == ring_pos + 1 || i - N_RING == ring_pos + 1) {
//      leds[ring_leds[i]] = CHSV(ring_hue, 255, 155);
//    } else if (i == ring_pos + 2 || i - N_RING == ring_pos + 2) {
//      leds[ring_leds[i]] = CHSV(ring_hue, 255, 105);
//    } else {
//      leds[ring_leds[i]] = CRGB::Black;
//    }
//  }
//
//  int32_t temp_ring_pos = ring_pos;
//  ring_pos += ring_pos - prev_ring_pos + ring_acc;
//  prev_ring_pos = temp_ring_pos;
//  if (ring_pos < 0) {
//    ring_pos += N_RING;
//  }
//  else if (ring_pos > N_RING - 1) {
//    ring_pos -= N_RING;
//  }

  FastLED.show();
  FastLED.delay(10);
}

uint8_t get_led_hue(uint16_t led_idx, uint8_t clust_idx) {
  uint16_t clust_hue = get_clust_hue(clust_idx);
  uint8_t hue_delta = get_led_hue_delta(led_idx);
  uint16_t hue = clust_hue + hue_delta;
  if (hue > 255) {
    hue -= 256;
  }
  return hue;
}

uint16_t get_clust_hue(uint8_t clust_idx) {
  uint16_t hue = lerp16by8(
    clust_hue_easing_from[clust_idx],
    clust_hue_easing_to[clust_idx],
    ease8InOutApprox(clust_hue_easing_frac)
  );
  return hue;
}

uint8_t get_led_hue_delta(uint16_t led_idx) {
  uint8_t hue = lerp8by8(
    led_hue_easing_from[led_idx],
    led_hue_easing_to[led_idx],
    ease8InOutApprox(led_hue_easing_frac)
  );
  return hue;
}

uint16_t get_random_hue() {
  uint16_t hue_width = high_hue - low_hue - CLUST_HUE_WIDTH;
  return random16(hue_width) + low_hue;
}
