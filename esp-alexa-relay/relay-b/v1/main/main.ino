#include "constants.h"
#include <ESP8266WiFi.h>
#include <Espalexa.h>
#include <arduino-timer.h>

String deviceName = DEVICENAME;
byte mac[6];

Espalexa espalexa;
void deviceHandler(uint8_t brightness);

Timer<1, millis, void *> timer;

void setup(void){
  pinReset();

  led(LOW);

  relay(LOW);
  delay(100);
  relay(LOW);

  delay(1000);

  if (connectWifi()) {
    delay(1000);
    espDevice();
  }
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

  espalexa.addDevice(deviceName, deviceHandler);
  espalexa.begin();

  log("ESP Device Setup Complete");
}

void deviceHandler(uint8_t brightness) {
  if (brightness > 0) {
    relayOn(brightness);

    if (INCHING) {
      timer.in(INCHINGMS, timerCallback);
    } 
  }else {
    relayOff();
  }
}

bool timerCallback(void *) {
  relayOff();
  log("Timer Called");
  return false;
}

void relay(int state) {
  digitalWrite(RELAY, state);
}

void relayOn(int level) {
  relay(HIGH);
  espalexa.getDevice(0)->setValue(level);
  led(LOW);
}

void relayOff() {
  relay(LOW);
  espalexa.getDevice(0)->setValue(0);
  led(HIGH);
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

void loop(void) {
  espalexa.loop();
  timer.tick();
}