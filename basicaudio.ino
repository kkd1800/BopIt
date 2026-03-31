#include "DFRobotDFPlayerMini.h"

DFRobotDFPlayerMini player;
const int busyPin = 2;

void waitForTrackToFinish() {
  // Wait until playback actually starts
  while (digitalRead(busyPin) == HIGH) {
  }

  // Wait until playback ends
  while (digitalRead(busyPin) == LOW) {
  }
}

void setup() {
  Serial.begin(9600);

  if (!player.begin(Serial)) {
    while (true) {
    }
  }

  pinMode(busyPin, INPUT_PULLUP);
  player.volume(25);

  player.play(1);
  waitForTrackToFinish();

  player.play(2);
  waitForTrackToFinish();

  player.play(3);
  waitForTrackToFinish();

  player.play(4);
  waitForTrackToFinish();
}

void loop() {
}