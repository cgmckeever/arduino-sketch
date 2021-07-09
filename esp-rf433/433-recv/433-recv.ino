#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin(115200);
  Serial.println("LISTEN...");

  // Arduino Pin2 = 0
  mySwitch.enableReceive(1);
}

void loop() {
  if (mySwitch.available()) {
    output(mySwitch.getReceivedValue(), mySwitch.getReceivedBitlength(),
      mySwitch.getReceivedDelay(), mySwitch.getReceivedRawdata(),
      mySwitch.getReceivedProtocol());
    mySwitch.resetAvailable();
  }
}
