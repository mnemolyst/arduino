#include "FastLED.h"

#define N_LEDS 290
#define BRIGHTNESS 255

CRGB leds[N_LEDS];
uint32_t t;

void setup() {
  delay(1000);
  t = millis();

  Serial.begin(9600);

  FastLED.addLeds<WS2812, 6>(leds, N_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  for (uint16_t i = 0; i < N_LEDS; i++) {
    leds[i] = CHSV(255, 255, 255);
  }
  FastLED.show();
}

void loop() {
  // static uint32_t dt;
  // dt = millis() - t;
  t = millis();

  uint16_t hue_start = ((t * 256) / 10000) % 256;
  while (hue_start > 255) {
    hue_start -= 256;
  }

  uint16_t hue;
  for (uint16_t i = 0; i < N_LEDS; i++) {
    hue = hue_start + ((i * 256) / 100);
    while (hue > 255) {
      hue -= 256;
    }
    leds[i] = CHSV(hue, 255, 255);
  }
  FastLED.show();
}
