#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin(115200);
  Serial.println("TRANSMIT...");

  // Arduino Pin10 = 10 ... no idea
  mySwitch.enableTransmit(10);
  
  /*
  mySwitch.setPulseLength(454);
  
  mySwitch.send(19077140, 26);
  delay(15000);

  mySwitch.send(19073044, 26);
  delay(15000);

  mySwitch.send(19075092, 26);
  delay(15000);
  
  mySwitch.send(35850260, 26);
  delay(15000);

  mySwitch.send(35854356, 26);
  delay(15000);

  mySwitch.send(35852308, 26);
  delay(15000);
  */

  mySwitch.setPulseLength(302);

  // Button 1
  mySwitch.send(10979556, 24);
  delay(5000);
  mySwitch.send(10979560, 24);

  // Button 2
  mySwitch.send(10979554, 24);
  delay(5000);
  mySwitch.send(10979553, 24);

  // Button 3
  mySwitch.send(10979557, 24);
  delay(5000);
  mySwitch.send(10979564, 24);
  
  
}

void loop() {
  
}
