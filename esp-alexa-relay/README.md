# ESP 8266 Alexa Enabled Relay

Creates a discoverable Philips Hue Device in Alexa using a ES8266 Chipset and relay.
Once discovered, Alexa can turn it on and off

## Python3 issue

- `env: python3: No such file or directory`

```
$ cd /Users/[your-username]/Library/Arduino15/packages/esp8266/tools/python3/3.7.2-post1/
$ rm python3
$ which python3
$ ln -s [path-returned-by-the-above-command] python3
```

## v1

### Prerequisites

- [ESP8266 WiFi 5V 1 Channel Relay Delay](https://www.amazon.com/gp/product/B071WWMMDD)
- [fauxmoesp](https://bitbucket.org/xoseperez/fauxmoesp/src/master/):  ESP8266/ESP32-based devices that emulates Philips Hue lights
- [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP): Async TCP Library for ESP8266 Arduino

### Steps

- Update constants 
- Flash the ESP 8266 Chip
- Once it connects to WIFI (blue light goes off), have alexa discover
- Connect your device
- Enjoy


