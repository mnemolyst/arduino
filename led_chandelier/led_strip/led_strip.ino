#include <pixelset.h>
#include <hsv2rgb.h>
#include <dmx.h>
#include <controller.h>
#include <pixeltypes.h>
#include <color.h>
#include <fastspi.h>
#include <bitswap.h>
#include <fastspi_bitbang.h>
#include <lib8tion.h>
#include <fastpin.h>
#include <led_sysdefs.h>
#include <power_mgt.h>
#include <fastspi_nop.h>
#include <fastspi_ref.h>
#include <noise.h>
#include <chipsets.h>
#include <fastled_progmem.h>
#include <fastled_config.h>
#include <cpp_compat.h>
#include <colorutils.h>
#include <fastled_delay.h>
#include <fastspi_types.h>
#include <fastspi_dma.h>
#include <colorpalettes.h>
#include <platforms.h>
#include <FastLED.h>

#define LED_PIN     5
#define NUM_LEDS    128
#define BRIGHTNESS  64
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

void setup() {
  delay( 3000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );

  //currentPalette = RainbowColors_p;
  SetupBlackAndWhiteStripedPalette();
  currentBlending = LINEARBLEND;
}

void loop() {
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */
  //SetupTotallyRandomPalette();
  FillLEDsFromPaletteColors( startIndex);
  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = 255;

    for( int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
    for( int i = 0; i < 16; i++) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
}
