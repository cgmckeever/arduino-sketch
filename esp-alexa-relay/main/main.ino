#include "constants.h"

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <fauxmoESP.h>

fauxmoESP fauxmo;

int pause = 1000;
unsigned long time_now = 0;
String deviceName = DEVICENAME;


void setup() { 
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW); 
  delay(2000);
  
  

  if (connectWifi()) {
     Serial.println("Adding device " + deviceName);
     fauxmo.setPort(80);  
     fauxmo.enable(true);
     fauxmo.addDevice(DEVICENAME);   
     digitalWrite(LED, HIGH); 
     pinMode(LED, INPUT);

     Serial.begin(9600);
     Serial.write(relOFF, sizeof(relOFF));
     Serial.flush();
  }

  
  fauxmo.onSetState([](unsigned char device_id, const char * device_name, 
                    bool state, unsigned char value) {
    if (state) {
      Serial.write(relON, sizeof(relON));
      Serial.flush();

      if (inching){
        // delay was not keeping the relay open long enough
        time_now = millis();
        while(millis() < time_now + pause){
          //wait approx. [pause] ms
        }
        Serial.write(relOFF, sizeof(relOFF));
      }
      
    }
    
  });
}

boolean connectWifi() {
  
  
  WiFi.begin(ssid, password);
  WiFi.hostname("fesp-" + deviceName);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(".......");
  Serial.println("WiFi Connected....IP Address: " + WiFi.localIP());

  return true;
}

void loop() {
  fauxmo.handle();
}
