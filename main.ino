#include <Arduino.h>
#include <oled.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <TM1637Display.h>
#include "bitmaps.h"
#include "DFRobotDFPlayerMini.h"

OLED OLED_display(A2, A3, NO_RESET_PIN, OLED::W_128, OLED::H_64, OLED::CTRL_SH1106, 0x3C);
TM1637Display timer_hex(10, 9);  // CLK=D10, DIO=D9
TM1637Display score_hex(12, 13);  // CLK=D10, DIO=D9
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);
DFRobotDFPlayerMini player;

// constant variables
static const int busyPin = 2;
static const int touchPin = 4;
static const int rotaryCLK = 5;
static const int direcEnc = 6;

// live variables
bool success = false;
uint32_t timeLimit = 5000;
uint32_t timeDelay = 1000;
uint8_t score = 0;
int8_t shieldLevel = 0;
int lastStateCLK;

// helper to pause until a track finishes
void waitForTrackToFinish() {
  // Wait until playback actually starts
  while (digitalRead(busyPin) == HIGH) {
  }

  // Wait until playback ends
  while (digitalRead(busyPin) == LOW) {
  }
}

void clearChannels(){
  for(int i=0; i<=5; i++){
    pca.setPin(i, 0);
  }
}

void setChannels(uint8_t n){
  clearChannels();
  for(int i=1; i<=n; i++){
    pca.setPin(i-1, percentTo12Bit(100));
  }
}

// Convert percent to 12 bit
uint16_t percentTo12Bit(uint8_t pct) {
  if (pct == 0)   return 0;
  if (pct >= 100) return 4095;
  return (uint16_t)((pct / 100.0f) * 4095);
}

// intro
void setup() {
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

  // initialize speaker
  Serial.begin(9600);

  if (!player.begin(Serial)) {
    while (true) {
    }
  }

  player.volume(15);
  pinMode(busyPin, INPUT_PULLUP);

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

  for (uint8_t row = 1; row <= 57; row++){
    
    if(row <= 54) loadingBar[row] = 25;
    if(row >= 2) loadingBar[row-1] = 29;
    if(row >= 3) loadingBar[row-2] = 31;

    OLED_display.draw_bitmap(65, 32, 56, 5,loadingBar);
    OLED_display.display();
  }

  // wait for button press to start game
  while(digitalRead(7) == HIGH){}

  OLED_display.clear();
  OLED_display.draw_bitmap_P(0, 0, 128, 64, targetingDisplay);
  OLED_display.display();
}

// main loop
void loop() {
  // update score
  score_hex.showNumberDec(score, true);

  // enter base state
  shieldLevel = 6;
  setChannels(shieldLevel);

  OLED_display.clear();
  OLED_display.draw_bitmap_P(0, 0, 128, 64, targetingDisplay);
  OLED_display.draw_bitmap_P(31, 19, 66, 26, xWing);
  OLED_display.draw_bitmap_P(18, 10, 92, 44, shield);
  OLED_display.display();

  // issue command
  delay(timeDelay);
  
  int choice = random(0, 2);  // 0 or 1

  if (choice == 0) {
    success = command_shields(timeLimit);
  } else {
    success = command_test(timeLimit);
  }

  // go to failure function if unsuccessful
  if(!success){
    failure();
    score = 0;
  }
  else score++;
}

bool command_test(uint32_t limit){
  // command cue
  player.play(3);
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

    // return true if input is detected
    if (digitalRead(touchPin) == LOW) {
      //sucess cue
      timer_hex.showNumberDec(0, true);
      player.play(1);

      // return success case
      return true;
    }

    current_time = millis() - start_time;
  }

  // return false if time runs out
  timer_hex.showNumberDec(0, true);
  return false;
}

bool command_shields(uint32_t limit){
  // command cue
  clearChannels();
  shieldLevel = 0;

  player.play(6);

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

    // return true if input is detected
    if (shieldLevel == 6) {
      //sucess cue
      timer_hex.showNumberDec(0, true);

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

      setChannels(shieldLevel);
    }

    // save the current state for next time
    lastStateCLK = currentStateCLK;

    current_time = millis() - start_time;
  }

  // return false if time runs out
  timer_hex.showNumberDec(0, true);
  return false;
}

void failure(){
  // hold in fail state until restart triggered
  clearChannels();
  player.loop(5);
  OLED_display.clear();
  OLED_display.draw_bitmap_P(0, 0, 128, 64, targetingDisplay);
  OLED_display.display();
  
  uint16_t row = 0;
  bool forward = true;

  while(digitalRead(7) == HIGH){
    pca.setPin(row, percentTo12Bit(100));
    if(forward){
      if(row == 5){
        row = 4;
        forward = false;
      }
      else row++;
    }
    else{
      if(row == 0){
        row = 1;
        forward = true;
      }
      else row--;
    }
    delay(50);
    clearChannels();
  }

  // leave fail state
}
