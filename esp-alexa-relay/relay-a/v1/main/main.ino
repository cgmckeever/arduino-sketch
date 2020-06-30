#include "constants.h"
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <fauxmoESP.h>
#include <arduino-timer.h>

fauxmoESP fauxmo;

Timer<1, millis, void *> timer;

String deviceName = DEVICENAME;
byte mac[6];

void setup() { 
  led(LOW);
  delay(1000)

  relayOff();
  delay(100);
  relayOff();

  if (connectWifi()) {
    espDevice();  
  }
}

boolean connectWifi() {
  log("");
  log("Serching for " + SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  WiFi.macAddress(mac); 
  String hostname = "fesp-" + deviceName + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  WiFi.hostname(hostname);
  
  while (WiFi.status() != WL_CONNECTED) {
    flash();
    delay(500);
  }

  led(HIGH); 
  log(hostname + " Connected - IP Address: " + WiFi.localIP().toString());

  return true;
}

void espDevice() {
  log("Adding Alexa device as " + deviceName);

  fauxmo.createServer(true);
  fauxmo.setPort(80);  
  fauxmo.enable(true);
  fauxmo.addDevice(DEVICENAME); 

  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value)  {
    deviceHandler(state);
  });

  log("ESP Device Setup Complete");
}

void deviceHandler(bool state) {
    if (state) {
      relayOn();
      
      if (INCHING) {
        timer.in(INCHINGMS, timerCallback);
      } 
    } else {
      relayOff();
    }
}

bool timerCallback(void *) {
  relayOff();
  log("Timer Called");
  return false;
}

void relay(const byte *state) {
  pinMode(LED, INPUT);
  Serial.begin(9600);
  Serial.write(state, sizeof(state));
  Serial.flush();
  Serial.end();
}

void relayOn() {
  relay(relON);
  led(LOW);
}

void relayOff() {
  relay(relOFF);
  led(HIGH);
}

void flash() {
  int state = digitalRead(LED) == HIGH ? LOW : HIGH;
  led(state);
}

void led(int state) {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, state);
}

void log(String msg) {
  Serial.begin(115200);
  Serial.println(msg);
  Serial.flush();
  Serial.end();
}

void loop() {
  fauxmo.handle();
  timer.tick();
}
