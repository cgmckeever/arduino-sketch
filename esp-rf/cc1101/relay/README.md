# ESP RF Relay

- Listens for Sensors using [rtl_433_esp](https://github.com/NorthernMan54/rtl_433_ESP) and can relay to a http listener (ie NodeRed)
- Runs a http web server that accept params for a [rc-switch](https://github.com/sui77/rc-switch) signal and transmits the switch code

## Control endpoint

- `/control`
- Params (these are obtained from listening with rc-switch)

```
?freq=433.92&
 pulse=389&
 decimal=16776961&
 bits=24
```


## Specs

- CC1101 RF Rx/Tx
- ESP32
