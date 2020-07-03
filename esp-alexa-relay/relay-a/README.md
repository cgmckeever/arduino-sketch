# ESP 8266 Alexa Enabled Relay

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

## v1

- Code currently working
- fauxmoesp is no longer supported 

## v2

- Built-in web based configuration and relay-control
- Requires [ConfigManager](https://github.com/nrwiersma/ConfigManager)




