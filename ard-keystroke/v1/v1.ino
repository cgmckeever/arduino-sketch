// https://www.arduino.cc/reference/en/language/functions/usb/keyboard/keyboardmodifiers/
//
#include "Keyboard.h"
const int shutdownCodes[] = { 128, 129, 130, 100};
const int sleepCodes[] = { 128, 129, 130, 101};
const int rebootCodes[] = { 128, 129, 130, 102};

const int pressTime = 100;

const int rxLed = 17;
const int txLed = 30;

const int shutdownPin = 2;
const int sleepPin = 3;
const int rebootPin = 4;

int shutdownStatePrevious = HIGH;
int sleepStatePrevious = HIGH;
int rebootStatePrevious = HIGH;

void setup() {
  Serial.begin(9600);

  pinMode(shutdownPin, INPUT_PULLUP);
  pinMode(sleepPin, INPUT_PULLUP);
  pinMode(rebootPin, INPUT_PULLUP);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(rxLed, OUTPUT);
  pinMode(txLed, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(rxLed, LOW);
  digitalWrite(txLed, LOW);

  Keyboard.begin();
}

void loop() {
  int shutdownState = digitalRead(shutdownPin);
  int sleepState = digitalRead(sleepPin);
  int rebootState = digitalRead(rebootPin);

  if (shutdownStatePrevious != shutdownState)
  {
    buttonChanged(shutdownState, shutdownPin);
    shutdownStatePrevious = shutdownState;
  }

  if (sleepStatePrevious != sleepState)
  {
    buttonChanged(sleepState, sleepPin);
    sleepStatePrevious = sleepState;
  }

  if (rebootStatePrevious != rebootState)
  {
    buttonChanged(rebootState, rebootPin);
    rebootStatePrevious = rebootState;
  }
}

void buttonChanged(int buttonState, int pin) {
  Serial.println(buttonState);

  if (buttonState == LOW) {
    Serial.println(pin);

    switch (pin) {
      case shutdownPin:
        keyAction(shutdownCodes, sizeof(shutdownCodes)/sizeof(int));
        break;
      case sleepPin:
        keyAction(sleepCodes, sizeof(sleepCodes)/sizeof(int));
        break;
      case rebootPin:
        keyAction(rebootCodes, sizeof(rebootCodes)/sizeof(int));
        break;
    }
  }
}

void keyAction(int codes[], int len) {
  for (int code = 0; code < len; code++) {
    Keyboard.press(codes[code]);
  }
  delay(pressTime);
  for (int code = 0; code < len; code++) {
    Keyboard.release(codes[code]);
  }
}


