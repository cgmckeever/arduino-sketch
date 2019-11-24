#include "constants.h"

#include <ESP8266WiFi.h>

const char* ap_ssid = "OwensManBun";
const char* ap_password = ""; 

WiFiServer server(80);


void setup() { 

  Serial.println("");
  Serial.begin(74880);
  delay(500);

  WiFi.mode(WIFI_AP_STA);

  sta_mode();
  ap_mode();
  

}



void loop() {
  //scan();
  delay(5000);
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.softAPIP());
}


/**
  

 **/

void scan() {
  Serial.println("scan start");

  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(") ");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(1000);
    }
  }
  
  Serial.println("");

  
}


void ap_mode() {
  WiFi.softAP(ap_ssid, ap_password);
  server.begin();
}


boolean sta_mode() {  
  Serial.println("Starting STA Mode");
  
  WiFi.hostname("Yoda");
  WiFi.begin(ssid, password);

  // IPAddress ip(192, 168, 24, 230); // this 3 lines for a fix IP-address
  // IPAddress gateway(192,168,24,1);
  // IPAddress subnet(255,255,254,0);
  // WiFi.config(ip, gateway, subnet);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println(".......");
  Serial.print("WiFi Connected....IP Address: ");
  Serial.println(WiFi.localIP());

  return true;
  
}
