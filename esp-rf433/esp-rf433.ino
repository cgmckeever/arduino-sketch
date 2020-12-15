#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

bool transmit = false;

void setup() {
  Serial.begin(115200);

  if (transmit) {
    Serial.println("TRANSMIT...");
    // Arduino Pin10 = 10 ... no idea
    mySwitch.enableTransmit(10);
    mySwitch.setPulseLength(509);
  } else {
    Serial.println("LISTEN...");
    // Arduino Pin2 = 0
    mySwitch.enableReceive(0);
  }

  
}

void loop() {
  if (transmit) {
    mySwitch.send(5153, 16); 
  } else {
    if (mySwitch.available()) {
      output(mySwitch.getReceivedValue(), mySwitch.getReceivedBitlength(), mySwitch.getReceivedDelay(), mySwitch.getReceivedRawdata(),mySwitch.getReceivedProtocol());
      mySwitch.resetAvailable();
    }
  }
}
