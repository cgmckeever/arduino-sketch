# ESP RF Relay

- Listens for Sensors using [rtl_433_esp](https://github.com/NorthernMan54/rtl_433_ESP) and can relay to a http listener (ie NodeRed)
- Runs a http web server that accept params for a [rc-switch](https://github.com/sui77/rc-switch) signal and transmits the switch code
- [PinOut](https://www.die-welt.net/2021/06/controlling-somfy-roller-shutters-using-an-esp32-and-esphome/)

## Passed Sensor params

```
payload: object
model: "Acurite-Rain899"
id: 3043
channel: 0
battery_ok: 1
rain_mm: 33.782
mic: "CHECKSUM"
protocol: "Acurite 592TXR Temp/Humidity, 5n1 Weather Station, 6045 Lightning, 3N1, Atlas"
rssi: -52
duration: 227014
```

## Switch Control endpoint

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
