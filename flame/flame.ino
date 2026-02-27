#define LED_COUNT 240
#include <Adafruit_NeoPixel.h>
#include "FastLED.h"

#define BRIGHTNESS 100

#define ENCODER_DIR   1
#define ENCODER_OUT_A 6 // CLK
#define ENCODER_OUT_B 7 // DT
#define ENCODER_OUT_C 8 // SW
#define LED_OUT       9

Adafruit_NeoPixel leds(LED_COUNT, LED_OUT, NEO_GRBW + NEO_KHZ800);

#define COIL_REP 20
#define N_CELLS  LED_COUNT + 2 * COIL_REP
#define CELLA_I  COIL_REP
#define CELLZ_I  CELLA_I + LED_COUNT - 1
#define OVERRELAX 1.9

// physical/math constants
#define G 9.81
#define LIFT_ACCEL 3
#define SIN30 0.5
#define SIN60 0.866

#define _B *256

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

  for (uint16_t i = 0; i < N_CELLS; i++) {
    u[i] = 0.0;
    v[i] = 0.0;
    w[i] = 0.0;
    new_u[i] = 0.0;
    new_v[i] = 0.0;
    new_w[i] = 0.0;
    t[i] = 0.0;
    new_t[i] = 0.0;
  }

  Serial.begin(9600);
}

// add gravity and lift forces
void integrate(uint32_t dt) {
  for (uint16_t i = CELLA_I; i <= CELLZ_I; i++) {
    float fall = -G * dt;
    float lift = t[i] * LIFT_ACCEL * dt;
    v[i] += (fall + lift) * SIN60;
    w[i] += (fall + lift) * SIN60;
  }
}

uint8_t get_s(uint16_t idx) {
  if (idx >= CELLA_I && idx <= CELLZ_I) {
    return 1;
  }
  return 0;
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
      float div = u[i] - u[i - 1] + v[i] - v[i - COIL_REP] + w[i] - w[i - COIL_REP + 1];
      float push = -div / s * OVERRELAX;
      u[i] -= push * su2;
      u[i - 1] += push * su1;
      v[i] -= push * sv2;
      v[i - COIL_REP] += push * sv1;
      w[i] -= push * sw2;
      w[i - COIL_REP + 1] += push * sw1;
    }
  }
}

void advect_velocity(uint32_t dt) {
}

void advect_temperature(uint32_t dt) {
}

void draw() {
  uint32_t color;
  for (uint16_t i = CELLA_I; i <= CELLZ_I; i++) {
    color = leds.ColorHSV((0 + t[i])_B, 255, 255);
    leds.setPixelColor(i, color);
  }
  leds.show();
}

void loop() {
  uint32_t d_time = millis() - last_t;
  last_t = millis();

  integrate(d_time);
  solve_incompressibility(3);
  // the original has a boundary-copying method here called extrapolate
  advect_velocity(d_time);
  advect_temperature(d_time);

  // limit the framerate
  if (millis() - last_draw_t > 30) {
    draw();
    last_draw_t = millis();
  }
}
