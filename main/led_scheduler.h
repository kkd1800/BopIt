#pragma once

#include <Adafruit_NeoPixel.h>
#include <Adafruit_PWMServoDriver.h>

#define NEO_PIN    11
#define NEO_COUNT  8
#define NEO_PIN_RGB  8
#define NEO_COUNT_RGB 16    // two 8-LED sticks chained = 16 total

enum LedPattern {
  EMPTY,
  START_WAIT,
  SHIELD_WAIT,
  BLASTERS_WAIT,
  HYPERDRIVE_WAIT,
  VICTORY,
  FAILURE
};

inline uint16_t percentTo12Bit(float percent) {
  return (uint16_t)(constrain(percent, 0.0, 100.0) / 100.0 * 4095);
}

extern Adafruit_NeoPixel stripRGBW;   // defined in .cpp, accessible everywhere
extern Adafruit_NeoPixel stripRGB;
extern Adafruit_PWMServoDriver pca;  // defined in led_scheduler.cpp

void setPattern(LedPattern p, uint16_t intervalMs = 40);
void ledTick();
