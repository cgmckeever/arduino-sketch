/*
  Example for different sending methods
  
  https://github.com/sui77/rc-switch/
  https://github.com/LSatan/SmartRC-CC1101-Driver-Lib
  ----------------------------------------------------------
  Mod by Little Satan. Have Fun!
  ----------------------------------------------------------
*/
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <RCSwitch.h>

int pin; // int for Transmit pin.
int wait = 10000;

RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin(115200);

#ifdef ESP32
pin = 2;  // for esp32! Transmit on GPIO pin 2.
#elif ESP8266
pin = 5;  // for esp8266! Transmit on pin 5 = D1
#else
pin = 6;  // for Arduino! Transmit on pin 6.
#endif

//CC1101 Settings:                (Settings with "//" are optional!)
  ELECHOUSE_cc1101.Init();            // must be set to initialize the cc1101!
//ELECHOUSE_cc1101.setRxBW(812.50);  // Set the Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.
//ELECHOUSE_cc1101.setPA(10);       // set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12)   Default is max!

  float freq = 433.92;
  //float freq = 315;
  ELECHOUSE_cc1101.setMHZ(freq); // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
 
  // Transmitter on 
  mySwitch.enableTransmit(pin);

  // cc1101 set Transmit on
  ELECHOUSE_cc1101.SetTx();
  
  // Optional set protocol (default is 1, will work for most outlets)
  // mySwitch.setProtocol(2);

  // Optional set pulse length.
  mySwitch.setPulseLength(453);
  
  // Optional set number of transmission repetitions.
  // mySwitch.setRepeatTransmit(15);

  Serial.print("Transmit...");
  Serial.println(freq);
}

void loop() {

  //mySwitch.send(19073044, 26);
  delay(wait);

  mySwitch.send(19075092, 26);
  delay(wait);

  mySwitch.send(19077140, 26);
  delay(wait);

  mySwitch.send(35850260, 26);
  delay(wait);

  mySwitch.send(35854356, 26);
  delay(wait);

  mySwitch.send(35852308, 26);
  delay(wait);

  

}
