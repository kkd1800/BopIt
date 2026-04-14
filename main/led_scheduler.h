#pragma once

#include <Adafruit_NeoPixel.h>

#define NEO_PIN    11
#define NEO_COUNT  8

enum LedPattern {
  LOADING,
  IDLE,
  SHIELD_CUE,
  SHIELD_SUCCESS,
  BLASTERS_CUE,
  BLASTERS_SUCCESS,
  HYPERDRIVE_CUE,
  HYPERDRIVE_SUCCESS,
  VICTORY,
  FAILURE
};

extern Adafruit_NeoPixel stripRGBW;   // defined in .cpp, accessible everywhere

void setPattern(LedPattern p, uint16_t intervalMs = 40);
void ledTick();