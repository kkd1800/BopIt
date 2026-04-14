#include "led_scheduler.h"   // ← your local header, not <angle brackets>

Adafruit_NeoPixel strip(NEO_COUNT, NEO_PIN, NEO_GRBW + NEO_KHZ800);

struct LedScheduler {
  LedPattern  pattern;        // current ambient pattern
  LedPattern  nextPattern;    // requested by game logic
  uint8_t     frame;          // animation frame counter
  uint32_t    nextUpdate;     // millis() timestamp for next tick
  uint16_t    interval;       // ms between frames
  bool        dirty;          // new pattern pending
};

LedScheduler ledSched = { PAT_IDLE, PAT_IDLE, 0, 0, 40, false };

// Call this from game logic — never call strip.show() directly
void setPattern(LedPattern p, uint16_t intervalMs = 40) {
  ledSched.nextPattern = p;
  ledSched.interval    = intervalMs;
  ledSched.dirty       = true;
}

// ── Per-pattern render functions ─────────────────────────────────────────

void renderIdle(uint8_t frame) {
  // Slow blue-white breathe across all pixels
  float breath = (sin(frame * 0.08) + 1.0) * 0.5;  // 0.0–1.0
  uint8_t w = (uint8_t)(breath * 60);
  uint8_t b = (uint8_t)(breath * 30);
  for (int i = 0; i < NEO_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, b, w));
  }
}

void renderShields(uint8_t frame) {
  // Cyan pulse sweeping left to right
  for (int i = 0; i < NEO_COUNT; i++) {
    float dist = abs((int)frame % NEO_COUNT - i);
    uint8_t v = max(0, 120 - (int)(dist * 40));
    strip.setPixelColor(i, strip.Color(0, v, v/2, 0));
  }
}

void renderBlasters(uint8_t frame) {
  // Rapid red-orange flicker
  for (int i = 0; i < NEO_COUNT; i++) {
    uint8_t flicker = random(60, 200);
    strip.setPixelColor(i, strip.Color(flicker, flicker / 4, 0, 0));
  }
}

void renderHyperspace(uint8_t frame) {
  // White streak from left to right, wrapping
  strip.clear();
  int pos = frame % NEO_COUNT;
  strip.setPixelColor(pos,               strip.Color(0, 0, 0, 255));
  strip.setPixelColor((pos + 1) % NEO_COUNT, strip.Color(0, 0, 0, 80));
  strip.setPixelColor((pos - 1 + NEO_COUNT) % NEO_COUNT, strip.Color(0, 0, 0, 30));
}

void renderSuccess(uint8_t frame) {
  // Green fill that pulses bright
  float pulse = (sin(frame * 0.2) + 1.0) * 0.5;
  uint8_t g = 80 + (uint8_t)(pulse * 150);
  for (int i = 0; i < NEO_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(0, g, 0, g / 4));
  }
}

void renderFailure(uint8_t frame) {
  // Red blink (frame-based, no delay())
  bool on = (frame / 6) % 2 == 0;
  for (int i = 0; i < NEO_COUNT; i++) {
    strip.setPixelColor(i, on ? strip.Color(180, 0, 0, 0) : strip.Color(0, 0, 0, 0));
  }
}

// ── Main tick — call this every loop() iteration ─────────────────────────

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

  strip.clear();
  switch (ledSched.pattern) {
    case PAT_IDLE:       renderIdle(ledSched.frame);       break;
    case PAT_SHIELDS:    renderShields(ledSched.frame);    break;
    case PAT_BLASTERS:   renderBlasters(ledSched.frame);   break;
    case PAT_HYPERSPACE: renderHyperspace(ledSched.frame); break;
    case PAT_SUCCESS:    renderSuccess(ledSched.frame);    break;
    case PAT_FAILURE:    renderFailure(ledSched.frame);    break;
  }
  strip.show();
}