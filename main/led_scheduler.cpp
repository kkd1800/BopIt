#include "led_scheduler.h"

Adafruit_NeoPixel stripRGBW(NEO_COUNT, NEO_PIN, NEO_GRBW + NEO_KHZ800);

struct LedScheduler {
  LedPattern  pattern;        // current ambient pattern
  LedPattern  nextPattern;    // requested by game logic
  uint8_t     frame;          // animation frame counter
  uint32_t    nextUpdate;     // millis() timestamp for next tick
  uint16_t    interval;       // ms between frames
  bool        dirty;          // new pattern pending
};

LedScheduler ledSched = { IDLE, IDLE, 0, 0, 40, false };

void setPattern(LedPattern p, uint16_t intervalMs = 40) {
  ledSched.nextPattern = p;
  ledSched.interval    = intervalMs;
  ledSched.dirty       = true;
}

//pattern render functions
void renderIdle(uint8_t frame) {
  // Cyan pulse sweeping left to right
  for (int i = 0; i < NEO_COUNT; i++) {
    float dist = abs((int)frame % NEO_COUNT - i);
    uint8_t v = max(0, 120 - (int)(dist * 40));
    stripRGBW.setPixelColor(i, stripRGBW.Color(0, v, v/2, 0));
  }
}

void renderVictory(uint8_t frame) {
  // Green fill that pulses bright
  float pulse = (sin(frame * 0.2) + 1.0) * 0.5;
  uint8_t g = 80 + (uint8_t)(pulse * 150);
  for (int i = 0; i < NEO_COUNT; i++) {
    stripRGBW.setPixelColor(i, stripRGBW.Color(0, g, 0, g / 4));
  }
}

void renderFailure(uint8_t frame) {
  // Red blink (frame-based, no delay())
  bool on = (frame / 6) % 2 == 0;
  for (int i = 0; i < NEO_COUNT; i++) {
    stripRGBW.setPixelColor(i, on ? stripRGBW.Color(180, 0, 0, 0) : stripRGBW.Color(0, 0, 0, 0));
  }
}

// function for updating RGBW Stick
void ledTick() {
  uint32_t now = millis();

  // Accept a new pattern immediately (resets frame counter)
  if (ledSched.dirty) {
    ledSched.pattern   = ledSched.nextPattern;
    ledSched.frame     = 0;
    ledSched.dirty     = false;
    ledSched.nextUpdate = now;   // render first frame right away
  }

  if (now < ledSched.nextUpdate) return;   // not time yet — exit instantly
  ledSched.nextUpdate = now + ledSched.interval;
  ledSched.frame++;

  stripRGBW.clear();
  switch (ledSched.pattern) {
    case IDLE:       renderIdle(ledSched.frame);       break;
    case VICTORY:       renderVictory(ledSched.frame);       break;
    case FAILURE:       renderFailure(ledSched.frame);       break;
  }
  stripRGBW.show();
}