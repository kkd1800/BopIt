#include "led_scheduler.h"

Adafruit_NeoPixel stripRGBW(NEO_COUNT, NEO_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel stripRGB(NEO_COUNT_RGB, NEO_PIN_RGB, NEO_GRB + NEO_KHZ800);
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);

struct LedScheduler {
  LedPattern  pattern;        // current ambient pattern
  LedPattern  nextPattern;    // requested by game logic
  uint8_t     frame;          // animation frame counter
  uint32_t    nextUpdate;     // millis() timestamp for next tick
  uint16_t    interval;       // ms between frames
  bool        dirty;          // new pattern pending
};

LedScheduler ledSched = { START_WAIT, START_WAIT, 0, 0, 40, false };

void setPattern(LedPattern p, uint16_t intervalMs = 40) {
  ledSched.nextPattern = p;
  ledSched.interval    = intervalMs;
  ledSched.dirty       = true;
}

//pattern render functions
void renderEmpty(uint8_t frame){
  for (int i = 0; i < NEO_COUNT; i++) {
    stripRGBW.setPixelColor(i, stripRGBW.Color(0, 0, 0, 0));
  }
  for (int i = 0; i < NEO_COUNT_RGB; i++) {
    stripRGB.setPixelColor(i, stripRGB.Color(0, 0, 0));
  }
}

void renderStartWait(uint8_t frame) {
  float pulse_g = (sin(frame * 0.25) + 1.0) * 0.5;
  float pulse_b = (sin(frame * 0.1) + 1.0) * 0.5;
  uint8_t g = 20 + (uint8_t)(pulse_g * 235);
  uint8_t b = (uint8_t)((pulse_b) * 255);
  for (int i = 0; i < NEO_COUNT; i++) {
    stripRGBW.setPixelColor(i, stripRGBW.Color(0, g, 0, 0));
  }
  for (int i = 0; i < NEO_COUNT_RGB; i++) {
    stripRGB.setPixelColor(i, stripRGB.Color(0, b, 255));
  }

  bool move = (frame / 3) % 1 == 0;

  if(move){
    for(int i = 6; i <= 11; i++){
      pca.setPin(i, 0);
    } 
    pca.setPin((((frame-3) / 3) % 6) + 6, percentTo12Bit(15));
    pca.setPin(((frame / 3) % 6) + 6, percentTo12Bit(100));
  }
}

void renderVictory(uint8_t frame) {
  // Green blink (frame-based, no delay())
  bool on = (frame / 6) % 2 == 0;
  for (int i = 0; i < NEO_COUNT; i++) {
    stripRGBW.setPixelColor(i, on ? stripRGBW.Color(0, 180, 0, 45) : stripRGBW.Color(0, 0, 0, 0));
  }
  // RGB sticks: matching green blink
  for (int i = 0; i < NEO_COUNT_RGB; i++) {
    stripRGB.setPixelColor(i, on ? stripRGB.Color(0, 180, 0) : stripRGB.Color(0, 0, 0));
  }
}

void renderFailure(uint8_t frame) {
  // Red pulse
  float pulse = (sin(frame * 0.05) + 1.0) * 0.5;
  uint8_t r = 20 + (uint8_t)(pulse * 235);
  for (int i = 0; i < NEO_COUNT; i++) {
    stripRGBW.setPixelColor(i, stripRGBW.Color(r, 0, 0, 0));
  }
  // RGB sticks: matching red pulse
  for (int i = 0; i < NEO_COUNT_RGB; i++) {
    stripRGB.setPixelColor(i, stripRGB.Color(r, 0, 0));
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
  stripRGB.clear();
  switch (ledSched.pattern) {
    case EMPTY:       renderEmpty(ledSched.frame);       break;
    case START_WAIT:       renderStartWait(ledSched.frame);    break;  
    case VICTORY:       renderVictory(ledSched.frame);       break;
    case FAILURE:       renderFailure(ledSched.frame);       break;
  }
  stripRGBW.show();
  stripRGB.show();
}
