/*
 Basic rtl_433_ESP example for OOK/ASK Devices
*/

#include <rtl_433_ESP.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define CC1101_FREQUENCY 433.92
#define JSON_MSG_BUFFER 512
#define LOG_LEVEL LOG_LEVEL_VERBOSE

#define RF_RECEIVER_GPIO 27
#define RF_EMITTER_GPIO 26

char messageBuffer[JSON_MSG_BUFFER];
int gdo0;

// use -1 to disable transmitter
rtl_433_ESP rf(-1);

#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();

void rtl_433_Callback(char* message) {
  DynamicJsonBuffer jsonBuffer2(JSON_MSG_BUFFER);
  JsonObject& RFrtl_433_ESPdata = jsonBuffer2.parseObject(message);
  logJson(RFrtl_433_ESPdata);
  rf.disableReceiver();
  mySwitch.enableReceive(RF_RECEIVER_GPIO);
}

void logJson(JsonObject &jsondata)
{
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  char JSONmessageBuffer[jsondata.measureLength() + 1];
#else
  char JSONmessageBuffer[JSON_MSG_BUFFER];
#endif
  jsondata.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Log.notice(F("Received message : %s" CR), JSONmessageBuffer);
}

void setup()
{
  Serial.begin(115200);

  gdo0 = RF_EMITTER_GPIO;

  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(CC1101_FREQUENCY);
  ELECHOUSE_cc1101.SetRx(CC1101_FREQUENCY);

  mySwitch.enableReceive(RF_RECEIVER_GPIO);

  Log.begin(LOG_LEVEL, &Serial);
  Log.notice(F(" " CR));
  Log.notice(F("****** setup ******" CR));
  rf.initReceiver(RF_RECEIVER_GPIO, CC1101_FREQUENCY);
  rf.setCallback(rtl_433_Callback, messageBuffer, JSON_MSG_BUFFER);
  rf.enableReceiver(RF_RECEIVER_GPIO);
  Log.notice(F("****** setup complete ******" CR));
}

void loop()
{
  rf.loop();

  if (mySwitch.available()) {
    output(mySwitch.getReceivedValue(), mySwitch.getReceivedBitlength(), mySwitch.getReceivedDelay(), mySwitch.getReceivedRawdata(),mySwitch.getReceivedProtocol());
    mySwitch.resetAvailable();
    rf.enableReceiver(RF_RECEIVER_GPIO);
  }
}