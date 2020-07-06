# ESP 8266 Alexa Enabled Relay

## Board Versions

- There seem to be many versions of this board. I have confirmed the one labeled as v3 (marking on back) works
out of the box with no module modification (outside custom ESP code). 

- There may be one version labeled [HW-655](https://www.youtube.com/watch?v=D470BrL15wA), that looks to work out of the box as well

- Finally, many knock-offs of this board exist. One idenitifcation of a knock-off is a continual LED flashing on the module
but the relay code does not work. There is a very [in-depth conversation](https://www.esp8266.com/viewtopic.php?f=160&t=13164) about this, as well as all over the rest of the internet. There is a fix to [flash the onboard-mcu (STC15F104W)](https://www.esp8266.com/viewtopic.php?f=160&t=13164&start=68#p74262). The referenced HEX code there does not appear to work, but another user posted an [updated version](https://www.esp8266.com/viewtopic.php?f=160&t=13164&start=96#p81907) that does work. Additionally, you can follow that thread on how to modify the board (remove the micrcontroler, solder some jumpers) and control it with one of the GPIO - this process is hard for the novice solderer.


## Prerequisites

- [ESP8266 WiFi 5V 1 Channel Relay Delay](https://amzn.to/3ga0q2c) (must be v3 - dual row of side pins)
- [fauxmoesp](https://bitbucket.org/xoseperez/fauxmoesp/src/master/):  ESP8266/ESP32-based devices that emulates Philips Hue lights
- [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP): Async TCP Library for ESP8266 Arduino

## Steps

- Update constants 
- Flash the ESP 8266 Chip
- Once it connects to WIFI (blue light goes off), have alexa discover
- Connect your device
- Enjoy

## Relay Operation 

The relay is controlled by sending serial commands to the on-board microprocessor. The commands are:

```
byte relON[] = {0xA0, 0x01, 0x01, 0xA2}; 
byte relOFF[] = {0xA0, 0x01, 0x00, 0xA1};

Serial.begin(9600);
Serial.write(relON, sizeof(relON));
Serial.flush();
Serial.end();

```

## v1

- Code currently working
- fauxmoesp is no longer supported 

## v2

- Built-in web based configuration and relay-control
- Requires [ConfigManager](https://github.com/nrwiersma/ConfigManager)




