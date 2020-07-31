//This code uses the fauxmoESP library
//lamp-becky-stern can be found at https://github.com/phodal/wdsm-code-pieces/blob/master/fauxmoESP-lamp-becky-stern.ino
//and was later modified by SilentDreamcast (to disable button and set pin to work with cheap Ebay relay boards)
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "fauxmoESP.h"
#include "ESPAsyncWebServer.h"
#include <ESPAsyncTCP.h>
#include <Hash.h>

#define WIFI_SSID "Soup_IOT"  //change this to your WIFI SSID
#define WIFI_PASS "12345678"  //change this to your WIFI password
#define SERIAL_BAUDRATE 115200

fauxmoESP fauxmo;
#define RELAY1 0

//const int  buttonPin = 4;    // the pin that the pushbutton is attached to
//int buttonState = 0;         // current state of the button
//int lastButtonState = 0;     // previous state of the button

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

void wifiSetup() {

    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);

    // Connect
    Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Wait
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println();

    // Connected!
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

void callback(uint8_t device_id, const char * device_name, bool state) {
  Serial.print("Device "); Serial.print(device_name); 
  Serial.print(" state: ");
  if (state) {
    Serial.println("ON");
    digitalWrite(RELAY1, HIGH);
  } else {
    Serial.println("OFF");
    digitalWrite(RELAY1, LOW);
  }
}

void setup() {
    pinMode(RELAY1, OUTPUT);
    //pinMode(buttonPin, INPUT_PULLUP);
    digitalWrite(RELAY1, LOW);
    // Init serial port and clean garbage
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println("FauxMo demo sketch");
    Serial.println("After connection, ask Alexa/Echo to 'turn <devicename> on' or 'off'");

    // Wifi
    wifiSetup();

    // Fauxmo
    fauxmo.addDevice("relay 3");
    fauxmo.onMessage(callback);
}

void loop() {
  fauxmo.handle();
  
  // read the pushbutton input pin:
  /*buttonState = digitalRead(buttonPin);

  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {
    // if the state has changed, increment the counter
    if (buttonState == LOW) {
      Serial.println("on");
      digitalWrite(RELAY1, HIGH);
    }
    else {
      // if the current state is LOW then the button
      // went from on to off:
      Serial.println("off");
      digitalWrite(RELAY1, LOW);
    }
    // Delay a little bit to avoid bouncing
    delay(50);
  }
  // save the current state as the last state,
  //for next time through the loop
  lastButtonState = buttonState;*/
}
