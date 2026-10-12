#include "FastLED.h"

#define BRIGHTNESS 50
//#define LOW_HUE 226
//#define HIGH_HUE 52
#define LOW_HUE 4
#define HIGH_HUE 36

#define N_LEDS 180

CRGB leds[N_LEDS];

void setup() {
  uint16_t hue;
  
  delay(1000);
  random16_set_seed(analogRead(0));
  
  FastLED.addLeds<NEOPIXEL, 9>(leds, N_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setCorrection(TypicalSMD5050);
  for (uint16_t i = 0; i < N_LEDS; i++) {
    hue = lerp8by8(LOW_HUE, HIGH_HUE, (i*255)/N_LEDS);
    if (hue > 255) {
      hue -= 256;
    }
    leds[i] = CHSV(hue, 255, 255);
  }
  FastLED.show();
}

void loop() {
}
