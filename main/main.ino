#include <Arduino.h>
#include <oled.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <TM1637Display.h>
#include "bitmaps.h"
#include "DFRobotDFPlayerMini.h"
#include <Adafruit_NeoPixel.h>
#include "led_scheduler.h"

OLED OLED_display(A2, A3, NO_RESET_PIN, OLED::W_128, OLED::H_64, OLED::CTRL_SH1106, 0x3C);
TM1637Display timer_hex(10, 9);  // CLK=D10, DIO=D9
TM1637Display score_hex(12, 13);  // CLK=D10, DIO=D9
DFRobotDFPlayerMini player;

// neopixel helper variables
static const uint32_t WHITE  = stripRGBW.Color(0,   0,   0,   255);
static const uint32_t OFF    = stripRGBW.Color(0,   0,   0,   0);
static const uint32_t RED    = stripRGBW.Color(255, 0,   0,   0);
static const uint32_t BLUE   = stripRGBW.Color(0,   0,  255, 0);
static const uint32_t ORANGE = stripRGBW.Color(200, 60,  0,   0);
static const uint32_t GREEN = stripRGBW.Color(0, 255,  0,   0);
static const uint32_t CYAN = stripRGBW.Color(0, 255,  255,   0);

static const uint32_t WHITE_RGB  = stripRGB.Color(0,   0,   0);
static const uint32_t OFF_RGB    = stripRGB.Color(0,   0,   0);
static const uint32_t RED_RGB    = stripRGB.Color(255, 0,   0);
static const uint32_t BLUE_RGB   = stripRGB.Color(0,   0,  255);
static const uint32_t ORANGE_RGB = stripRGB.Color(200, 60,  0);
static const uint32_t GREEN_RGB = stripRGB.Color(0, 255,  0);
static const uint32_t CYAN_RGB = stripRGB.Color(0, 255,  255,   0);

// constant variables
static const int busyPin = 2;
static const int touchPin = 4;
static const int rotaryCLK = 5;
static const int direcEnc = 6;

// live variables
bool success = false;
uint32_t timeLimit = 5200;
uint32_t timeDelay = 1025;
uint8_t score = 0;
int8_t shieldLevel = 0;
int lastStateCLK;

// Fill entire strip with one color
void fillAll(uint32_t color) {
    stripRGBW.fill(color);
    stripRGBW.show();
}

void fillAll_RGB(uint32_t color) {
    stripRGB.fill(color);
    stripRGB.show();
}

// Light a single pixel
void singlePixel(int index, uint32_t color) {
    stripRGBW.clear();
    stripRGBW.setPixelColor(index, color);
    stripRGBW.show();
}

void singlePixel_RGB(int index, uint32_t color) {
    stripRGB.setPixelColor(index, color);
    stripRGB.show();
}

// Wipe color across strip pixel by pixel
void colorWipe(uint32_t color, int delayMs) {
    for (int i = 0; i < stripRGBW.numPixels(); i++) {
        stripRGBW.setPixelColor(i, color);
        stripRGBW.show();
        delay(delayMs);
    }
}

// Chase: single lit pixel travelling along strip
void chase(uint32_t color, int delayMs) {
    for (int i = 0; i < stripRGBW.numPixels(); i++) {
        stripRGBW.clear();
        stripRGBW.setPixelColor(i, color);
        stripRGBW.show();
        delay(delayMs);
    }
}

void chase_RGB(uint32_t color, int delayMs) {
    for (int i = 0; i < stripRGBW.numPixels(); i++) {
        stripRGB.clear();
        stripRGB.setPixelColor(i, color);
        stripRGB.setPixelColor(15-i, color);
        stripRGB.show();
        delay(delayMs);
    }
}

// Blink entire strip n times
void blink(uint32_t color, int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        fillAll(color);
        delay(delayMs);
        fillAll(OFF);
        delay(delayMs);
    }
}

// Fade in from off to full brightness
void fadeIn(uint32_t color, int steps, int delayMs) {
    for (int b = 0; b <= 100; b += (100 / steps)) {
        stripRGBW.setBrightness(b);
        stripRGBW.fill(color);
        stripRGBW.show();
        delay(delayMs);
    }
    stripRGBW.setBrightness(80); // restore default
}

void fadeIn_RGB(uint32_t color, int steps, int delayMs) {
    for (int b = 0; b <= 30; b += (30 / steps)) {
        stripRGB.setBrightness(b);
        stripRGB.fill(color);
        stripRGB.show();
        delay(delayMs);
    }
    stripRGBW.setBrightness(30); // restore default
}

// helper to pause until a track finishes
void waitForTrackToFinish() {
  // Wait until playback actually starts
  while (digitalRead(busyPin) == HIGH) {
  }

  // Wait until playback ends
  while (digitalRead(busyPin) == LOW) {
  }
}

// helper to check if a track is done
bool trackPlaying() {
  if(digitalRead(busyPin) == LOW){
    return true;
  }
  else{
    return false;
  }
}

// helper to clear PCA channels
void clearChannels(){
  for(int i=0; i<=11; i++){
    pca.setPin(i, 0);
  }
}

// helper to set PCA channels
void setShields(uint8_t n){
  for(int i=0; i<=5; i++){
    pca.setPin(i, 0);
  }
  for(int i=1; i<=n; i++){
    pca.setPin(i-1, percentTo12Bit(100));
  }
}

// intro
void setup() {
  // neopixel stick setup
  stripRGBW.begin();
  stripRGBW.clear();
  stripRGBW.show();
  stripRGBW.setBrightness(100);
  stripRGB.begin();
  stripRGB.clear();
  stripRGB.show();
  stripRGB.setBrightness(30);

  // setup pins
  pinMode(7, INPUT_PULLUP);
  pinMode(touchPin, INPUT);
  pinMode(rotaryCLK, INPUT_PULLUP);
  pinMode(direcEnc, INPUT_PULLUP);
  lastStateCLK = digitalRead(rotaryCLK);

  // initialize PCA
  Wire.begin();
  Serial.begin(9600);
  pca.begin();
  pca.setPWMFreq(1000);
  clearChannels();

  pca.setPin(7, percentTo12Bit(100));
  pca.setPin(6, 0);
  pca.setPin(9, percentTo12Bit(100));
  pca.setPin(8, 0);
  pca.setPin(11, percentTo12Bit(100));
  pca.setPin(10, 0);

  // initialize speaker
  Serial.begin(9600);

  if (!player.begin(Serial)) {
    while (true) {
    }
  }

  pinMode(busyPin, INPUT_PULLUP);
  player.volume(28);
  player.play(19);

  // initialize hex displays
  timer_hex.setBrightness(4);
  timer_hex.showNumberDec(0, true);

  score_hex.setBrightness(4);
  score_hex.showNumberDec(score, true);

  // initialize OLED display
  OLED_display.begin();
  delay(1000);

  OLED_display.clear();
  OLED_display.draw_bitmap_P(0, 0, 128, 64, targetingDisplay);
  OLED_display.display();

  OLED_display.draw_bitmap_P(0, 0, 128, 64,welcomeScreen);
  OLED_display.display();

  // animate loading bar
  uint8_t loadingBar[56];
  memcpy_P(loadingBar, loadingBar_template, 56);  // copy flash → RAM

  fadeIn_RGB(BLUE_RGB, 30, 25);

  for (uint8_t row = 1; row <= 19; row++){
    if((row % 2) == 0){
      pca.setPin(6, percentTo12Bit(100));
      pca.setPin(7, 0);
      pca.setPin(8, percentTo12Bit(100));
      pca.setPin(9, 0);
      pca.setPin(10, percentTo12Bit(100));
      pca.setPin(11, 0);
      fillAll_RGB(BLUE_RGB);
    } else{
      pca.setPin(7, percentTo12Bit(100));
      pca.setPin(6, 0);
      pca.setPin(9, percentTo12Bit(100));
      pca.setPin(8, 0);
      pca.setPin(11, percentTo12Bit(100));
      pca.setPin(10, 0);
      fillAll_RGB(CYAN_RGB);
    }

    if(((row - 3) % 3) == 0) {
      shieldLevel++;
      setShields(shieldLevel);
    }

    if(row <= 18) loadingBar[3*row] = 25;

    loadingBar[3*row-1] = 29;
    loadingBar[3*row-2] = 31;
    loadingBar[3*row-3] = 31;

    if(row >= 2) {
      loadingBar[3*row-4] = 31;
      loadingBar[3*row-5] = 31;
    }

    OLED_display.draw_bitmap(65, 32, 56, 5,loadingBar);
    OLED_display.display();
  }

  // wait for button press to start game
  player.volume(15);
  player.play(18);
  setPattern(START_WAIT);

  while(digitalRead(7) == HIGH){
    ledTick();
  }

  OLED_display.clear();
  OLED_display.draw_bitmap_P(0, 0, 128, 64, targetingDisplay);
  OLED_display.display();
}

// main loop
void loop() {
  ledTick();

  // update displays & delay
  score_hex.showNumberDec(score, true);

  if((score % 5) == 0){
    timeLimit = timeLimit - 200;
    timeDelay = timeDelay - 25;
  }
  timer_hex.showNumberDec(timeLimit, true);

  // enter base state
  shieldLevel = 6;
  setShields(shieldLevel);
  setPattern(EMPTY);

  for(int i = 6; i <= 11; i++) pca.setPin(i, 0);

  OLED_display.clear();
  OLED_display.draw_bitmap_P(0, 0, 128, 64, targetingDisplay);
  OLED_display.draw_bitmap_P(31, 19, 66, 26, xWing);
  OLED_display.draw_bitmap_P(18, 10, 92, 44, shield);
  OLED_display.display();

  // delay while maintaining pattern
  for(int i = 0; i < timeDelay;){
    ledTick();
    delay(10);
    i = i + 10;
  }
  
  int choice = random(0, 3);  // 0 or 1

  if (choice == 0) {
    success = command_shields(timeLimit);
  } else if(choice ==1) {
    success = command_blaster(timeLimit);
  } else{
    success = command_hyperdrive(timeLimit);
  }

  // go to failure function if unsuccessful
  if(!success){
    failure();
    score = 0;
    timeLimit = 5200;
  }
  else if(score >= 5){
    victory();
    score = 0;
    timeLimit = 5200;
  }
  else score++;
}

bool command_hyperdrive(uint32_t limit){
  // command cue
  player.play(14);
  OLED_display.clear();
  OLED_display.draw_bitmap_P(0, 0, 128, 64, targetingDisplay);
  OLED_display.draw_bitmap_P(0,0,128,64, hyperdriveTarget);
  OLED_display.display();

  // set & display times
  uint32_t start_time = millis();
  uint32_t current_time = 0;
  timer_hex.showNumberDec(limit, true);

  // check for input while decrementing timer
  while(current_time < limit){
    timer_hex.showNumberDec(limit - current_time, true);
    ledTick();

    // return true if input is detected
    if (digitalRead(touchPin) == LOW) {
      //sucess cue

      player.play(15);

      fadeIn(WHITE, 20, 100);
      stripRGBW.clear();
      stripRGBW.show();
      delay(300);
      stripRGBW.setBrightness(100);
      fillAll(WHITE);
      while(trackPlaying()){}
      stripRGBW.setBrightness(80);

      // return success case
      return true;
    }

    current_time = millis() - start_time;
  }

  // return false if time runs out
  timer_hex.showNumberDec(0, true);

  // failure cue
  player.play(7);
  waitForTrackToFinish();

  player.play(8);
  waitForTrackToFinish();

  return false;
}

bool command_blaster(uint32_t limit){
  // command cue
  player.play(2);
  OLED_display.clear();
  OLED_display.draw_bitmap_P(0, 0, 128, 64, targetingDisplay);
  OLED_display.draw_bitmap_P(43,7,42,48,tieFighter);
  OLED_display.display();

  // set & display times
  uint32_t start_time = millis();
  uint32_t current_time = 0;
  timer_hex.showNumberDec(limit, true);

  // check for input while decrementing timer
  while(current_time < limit){
    timer_hex.showNumberDec(limit - current_time, true);
    ledTick();

    // return true if input is detected
    if (digitalRead(7) == LOW) {
      //sucess cue
      player.play(12);
      for (int i = 0; i < 3; i++) {
        fillAll(RED);
        chase_RGB(RED, 14);
        fillAll_RGB(OFF_RGB);
        fillAll(OFF);
        delay(115);
      }
      while(trackPlaying()){}

      player.play(13);

      // return success case
      return true;
    }

    current_time = millis() - start_time;
  }

  // return false if time runs out
  timer_hex.showNumberDec(0, true);

  // failure cue
  player.play(10);
  waitForTrackToFinish();

  player.play(9);
  blink(GREEN, 2, 115);
  while(trackPlaying()){}

  player.play(8);
  waitForTrackToFinish();

  return false;
}

bool command_shields(uint32_t limit){
  // command cue
  clearChannels();
  shieldLevel = 0;

  player.play(5);

  OLED_display.clear();
  OLED_display.draw_bitmap_P(0, 0, 128, 64, targetingDisplay);
  OLED_display.draw_bitmap_P(31, 19, 66, 26, xWing);
  OLED_display.display();

  // set & display times
  uint32_t start_time = millis();
  uint32_t current_time = 0;
  timer_hex.showNumberDec(limit, true);
  lastStateCLK = digitalRead(rotaryCLK);

  // check for input while decrementing timer
  while(current_time < limit){
    timer_hex.showNumberDec(limit - current_time, true);
    ledTick();

    // return true if input is detected
    if (shieldLevel == 6) {
      //sucess cue

      // return success case
      return true;
    }

    // rotary encoder logic
    int currentStateCLK = digitalRead(rotaryCLK);

    if (currentStateCLK != lastStateCLK) {

      if (digitalRead(direcEnc) != currentStateCLK) {
        shieldLevel = shieldLevel + 1;
      } else {
        shieldLevel = shieldLevel - 1;
      }

      if (shieldLevel > 6) shieldLevel = 6;
      if (shieldLevel < 0) shieldLevel = 0;

      setShields(shieldLevel);
    }

    // save the current state for next time
    lastStateCLK = currentStateCLK;

    current_time = millis() - start_time;
  }

  // return false if time runs out
  timer_hex.showNumberDec(0, true);

  // failure cue


  return false;
}

void failure(){
  // hold in fail state until restart triggered
  setPattern(FAILURE);
  clearChannels();
  OLED_display.clear();
  OLED_display.draw_bitmap_P(0, 0, 128, 64, targetingDisplay);
  OLED_display.draw_bitmap_P(41, 9, 46, 46, imperialLogo);
  OLED_display.display();
  player.loop(4);

  while(digitalRead(7) == HIGH){
    ledTick();
  }

  // leave fail state
  player.stop();
}

void victory(){
  // hold in victory state until restart triggered
  setPattern(VICTORY);
  clearChannels();
  player.loop(3);
  OLED_display.clear();
  OLED_display.draw_bitmap_P(0, 0, 128, 64, targetingDisplay);
  OLED_display.display();

  while(digitalRead(7) == HIGH){
    ledTick();
  }

  // leave victory state
  player.stop();
}
