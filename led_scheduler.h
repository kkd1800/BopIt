#pragma once

#include <Adafruit_NeoPixel.h>

#define NEO_PIN    11
#define NEO_COUNT  8

enum LedPattern {
  PAT_IDLE,
  PAT_SHIELDS,
  PAT_BLASTERS,
  PAT_HYPERSPACE,
  PAT_SUCCESS,
  PAT_FAILURE
};

extern Adafruit_NeoPixel strip;   // defined in .cpp, accessible everywhere

void setPattern(LedPattern p, uint16_t intervalMs = 40);
void ledTick();