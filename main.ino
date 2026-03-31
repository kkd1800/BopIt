#include <Arduino.h>
#include <oled.h>
#include <Wire.h>
#include <PCA9685.h>
#include <TM1637Display.h>
#include "bitmaps.h"

OLED OLED_display(A2, A3, NO_RESET_PIN, OLED::W_128, OLED::H_64, OLED::CTRL_SH1106, 0x3C);
TM1637Display timer_hex(10, 9);  // CLK=D10, DIO=D9
TM1637Display score_hex(12, 13);  // CLK=D10, DIO=D9
PCA9685 pca9685;

//variables
bool success = false;
uint32_t time_limit = 5000;
uint32_t start_time = 0;
uint32_t end_time = 0;
uint32_t remaining_time = 0;
uint8_t touchPin = 4;
uint8_t score = 0;

// helper to turn a PCA9685 channel fully on or off
void setLED(uint8_t channel, bool state) {
  pca9685.setChannelDutyCycle(channel, state ? 100 : 0);
}

//intro
void setup() {
  // setup pins
  pinMode(7, INPUT_PULLUP);
  pinMode(touchPin, INPUT);

  // initialize PCA9685
  pca9685.setupSingleDevice(Wire, 0x40);
  pca9685.setToFrequency(1000);

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

  // refresh OLED
  OLED_display.clear();
  OLED_display.draw_bitmap_P(0, 0, 128, 64, targetingDisplay);
  OLED_display.draw_bitmap_P(43,7,42,48,tieFighter);
  OLED_display.display();
}

// main loop
void loop() {
  score_hex.showNumberDec(score, true);
  delay(1000);

  // set start time and issue command
  start_time = millis();
  success = command(start_time, time_limit);

  // go to failure function if unsuccessful
  if(!success){
    failure();
  }
  
  score++;
}

bool command(uint32_t start_time, uint32_t limit){
  // set time and display timer
  uint32_t current_time = millis() - start_time;
  timer_hex.showNumberDec(limit, true);

  // check for input while decrementing timer
  while(current_time < limit){
    timer_hex.showNumberDec(limit - current_time, true);

    // return true if correct input is detected
    if (digitalRead(touchPin) == LOW) {
      timer_hex.showNumberDec(0, true);
      return true;
    }

    current_time = millis() - start_time;
  }

  // return false if time runs out
  timer_hex.showNumberDec(0, true);
  return false;
}

void failure(){
  //turn on failure LED and loop infinitely
  setLED(0, true);
  while(1){}
}
