#include "constants.h"
#include <ESP8266WiFi.h>
#include <Espalexa.h>

String deviceName = DEVICENAME;
byte mac[6];

Espalexa espalexa;
void deviceHandler(uint8_t brightness);

void setup(void){
  pinReset();

  led(LOW);
  delay(1000);

  if (connectWifi()) {
    delay(1000);
    espDevice();
  }

  relay(LOW);
  delay(100);
  relay(LOW);
}

void pinReset() {
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
}

boolean connectWifi() {
  WiFi.begin(SSID, PASSWORD);

  WiFi.macAddress(mac); 
  String hostname = "fesp-" + deviceName + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  WiFi.hostname(hostname);
  
  while (WiFi.status() != WL_CONNECTED) {
    flash();
    delay(500);
  }

  led(HIGH); 
  log("");
  log(hostname + " Connected - IP Address: " + WiFi.localIP().toString());

  return true;
}

void espDevice() {
  log("Adding Alexa device as " + deviceName);

  espalexa.addDevice(DEVICENAME, deviceHandler);
  espalexa.begin();

  log("ESP Device Setup Complete");
}

void deviceHandler(uint8_t brightness) {
  if (brightness > 0) {
    relayOn();

    if (inching){
      unsigned long timeNow = millis();
      while(millis() < timeNow + INCHINGTIME){
        // delay was not keeping the relay open long enough
        // wait approx. [pause] ms
      }
      relayOff();
    } 
  }
}

void relay(int state) {
  int ledState = state == HIGH ? LOW : HIGH;
  digitalWrite(RELAY, state);
  led(ledState);
}

void relayOn() {
  relay(HIGH);
}

void relayOff() {
  relay(LOW);
}

void flash() {
  int state = digitalRead(LED) == HIGH ? LOW : HIGH;
  led(state);
}

void led(int state) {
  digitalWrite(LED, state);
}

void log(String msg) {
  Serial.begin(115200);
  Serial.println(msg);
  Serial.flush();
  Serial.end();
  pinReset();
}

void loop(void){
  espalexa.loop();
}