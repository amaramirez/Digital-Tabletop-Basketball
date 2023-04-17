// This was written for the ATTiny85, interrupts on the Uno may be different

#include <EEPROM.h>
#include "segment.h"

// 74HC595 serial pins
int dataPin = 0;
int clockPin = 2;
int latchPin = 1;

// Button pin to start game
int modPin = 4;

// Flying fish or avoid sensor
int hoopPin = 3;
byte score = 0;
byte highScore = 0;
byte hoopCooldown = 75; // Prevents the interrupt from firing multiple times
unsigned long prevHoop = 0;

bool gameOver = true;
unsigned int gameTime = 60000;
unsigned long gameInitTime = 0;
unsigned long nextTime = 0;

// Interrupt for the hoop
ISR (PCINT0_vect) {
  // If the hoop pin goes low, check time and game state
  if (digitalRead(hoopPin) == LOW && nextTime - prevHoop > hoopCooldown && !gameOver) {
    // Add to score and reinit cooldown
    score++;
    prevHoop = nextTime;
  }
}

// Hard coded blinking for the score at the end
void doEndGame() {
  bool newHighScore = score > highScore;
  // Save new high score in EEPROM
  if (newHighScore) {
    EEPROM.write(22, score);
    highScore = score;
  }
  for (int i = 0; i < 3; i++) {
    // Display score[s]
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, segmentMap[score % 10]);
    shiftOut(dataPin, clockPin, MSBFIRST, segmentMap[(int)floor(score / 10)%10]);
    // If there is a new high score, display that on the left screen
    // Otherwise, turn it off
    shiftOut(dataPin, clockPin, MSBFIRST, newHighScore ? segmentMap[score % 10] : 0B11111111);
    shiftOut(dataPin, clockPin, MSBFIRST, newHighScore ? segmentMap[(int)floor(score / 10)] : 0B11111111);
    shiftOut(dataPin, clockPin, MSBFIRST, 0B00000000);
    digitalWrite(latchPin, HIGH);
    delay(500);
    // Turn off all displays
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 0B11111111);
    shiftOut(dataPin, clockPin, MSBFIRST, 0B11111111);
    shiftOut(dataPin, clockPin, MSBFIRST, 0B11111111);
    shiftOut(dataPin, clockPin, MSBFIRST, 0B11111111);
    shiftOut(dataPin, clockPin, MSBFIRST, 0B11110000);
    digitalWrite(latchPin, HIGH);
    delay(500);
  }
  score = 0;
}

// Display data on screen
void render() {
  // Loop four times for the top display
  for (int i = 0; i < 4; i++) {
    // Calculate game time and shorten it by 1 digit, because display is 4 digits
    unsigned int currentGameTime = (gameTime - (nextTime - gameInitTime))/10;
    // Convert each digit to value between 1-10, from left to right
    unsigned int timeDec = (currentGameTime/(int)pow(10, 3-i) )%10;
    digitalWrite(latchPin, LOW);
    // Current score render data, converting digits again
    shiftOut(dataPin, clockPin, MSBFIRST, segmentMap[score % 10]);
    shiftOut(dataPin, clockPin, MSBFIRST, segmentMap[(int)floor(score / 10)]);
    // High score render data
    shiftOut(dataPin, clockPin, MSBFIRST, segmentMap[highScore % 10]);
    shiftOut(dataPin, clockPin, MSBFIRST, segmentMap[(int)floor(highScore / 10)]);
    // Game time render data, one digit at a time
    // Left nibble controls ground to display, right nibble is the current digit
    shiftOut(dataPin, clockPin, MSBFIRST, (0B11110000 | timeDec) ^ (1 << i+4));
    digitalWrite(latchPin, HIGH);
  }
}

void setup() {
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);

  // ATTiny flushes all EEPROM values to 255 when uploading
  // overwrite if necessary
  if (EEPROM.read(22) == 255) EEPROM.write(22, 0);
  delay(100);

  // Read high score data into memory
  highScore = EEPROM.read(22);

  // Interrupt initialization, not sure how it works entirely
  cli();
  PCMSK |= bit (PCINT3);
  GIFR  |= bit (PCIF);
  GIMSK |= bit (PCIE);
  pinMode(hoopPin, INPUT_PULLUP);
  pinMode(modPin, INPUT);
  sei();

  // Blank out display
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, 0B11111111);
  shiftOut(dataPin, clockPin, MSBFIRST, 0B11111111);
  shiftOut(dataPin, clockPin, MSBFIRST, 0B11111111);
  shiftOut(dataPin, clockPin, MSBFIRST, 0B11111111);
  shiftOut(dataPin, clockPin, MSBFIRST, 0B11110000);
  digitalWrite(latchPin, HIGH);
  // Initialize counter
  nextTime = millis();
  gameInitTime = millis();
}

void loop() {
  // Don't update time unless game is active
  if (!gameOver) {
    nextTime = millis();
  }
  render();
  // Start game only if it is not already active
  if (digitalRead(modPin) == LOW && gameOver) {
    gameOver = false;
    gameInitTime = millis();
    nextTime = gameInitTime;
  }
  // Game over trigger
  if (nextTime - gameInitTime > gameTime) {
    gameOver = true;
    gameInitTime = nextTime;
    doEndGame();
  }
}