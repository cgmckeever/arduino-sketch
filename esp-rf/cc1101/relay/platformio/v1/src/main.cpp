#include <ArduinoLog.h>
#include <Arduino.h>
#include <HTTPClient.h>

#include <ConfigManager.h>
ConfigManager configManager;

#include <ArduinoQueue.h>
struct switchCommand {
    float freq;
    int pulse;
    int decimal;
    int bits;
};
typedef struct switchCommand SwitchCommand;
ArduinoQueue<switchCommand> switchCommandQueue(5);

#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <rtl_433_ESP.h>
rtl_433_ESP rf;

#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();

#include <main.h>

#define logLevel LOG_LEVEL_VERBOSE

// ConfigManger
//
const char *controlHTML = (char *)"/control.html";
const char *settingsHTML = (char *)"/settings.html";
const char *resetHTML = (char *)"/reset.html";
const char *stylesCSS = (char *)"/styles.css";
const char *mainJS = (char *)"/main.js";

const int deviceNameLen = 32;
const int serverURLLen = 32;

struct Config {
  char deviceName[deviceNameLen];
  char serverURL[serverURLLen];
  float frequency;
  int8_t receivePin;
  int8_t transmitPin;
} config;

struct Metadata {
} meta;

unsigned long previousMillis = 0;
unsigned long interval = 30000;

void configSetup() {
  DEBUG_MODE = true;

  String sApName = "ESPxxxx";

  configManager.setAPName(sApName.c_str());
  configManager.setAPFilename("/index.html");
  configManager.setWebPort(8080);

  // Config
  configManager.addParameter("deviceName", config.deviceName, deviceNameLen);
  configManager.addParameter("serverURL", config.serverURL, serverURLLen);
  configManager.addParameter("frequency", &config.frequency);
  configManager.addParameter("receivePin", &config.receivePin);
  configManager.addParameter("transmitPin", &config.transmitPin);

  // Callbacks
  configManager.setAPCallback(APCallback);
  configManager.setAPICallback(APICallback);
  configManager.begin(config);

  setConfigDefaults();
  printConfig();
}

void setConfigDefaults() {
  bool requireSave = false;

  char firstChar = config.deviceName[0];
  if (firstChar == '\0' || config.deviceName == NULL || firstChar == '\xFF') {
    strncpy(config.deviceName, "esp-device", deviceNameLen);
    requireSave = true;
  }

  firstChar = config.serverURL[0];
  if (firstChar == '\0' || config.serverURL == NULL || firstChar == '\xFF') {
    strncpy(config.serverURL, SERVER_URL, serverURLLen);
    requireSave = true;
  }

  if (float(config.frequency) < 0 || isnan(config.frequency)) {
    config.frequency = RF_MODULE_FREQUENCY;
    requireSave = true;
  }

  if (int(config.receivePin) < 0) {
    config.receivePin = RF_MODULE_GDO2;
    requireSave = true;
  }

  if (int(config.transmitPin) < 0) {
    config.transmitPin = RF_MODULE_GDO0;
    requireSave = true;
  }

  if (requireSave) configManager.save();
}

void printConfig() {
  Log.notice(F("====================" CR));
  Log.notice(F("Configuration" CR));
  Log.notice(F("Device Name : %s" CR), config.deviceName);
  Log.notice(F("Rx Pin : %s" CR), String(config.receivePin));
  Log.notice(F("Tx Pin : %s" CR), String(config.transmitPin));
  Log.notice(F("Freq : %s" CR), String(config.frequency));
  Log.notice(F("" CR));
}

void serveAssets(WebServer *server) {
  server->on("/styles.css", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(stylesCSS, mimeCSS);
  });

  server->on("/main.js", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(mainJS, mimeJS);
  });
}

void APCallback(WebServer *server) {
    serveAssets(server);
}

void APICallback(WebServer *server) {
  serveAssets(server);

  server->on("/config", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(settingsHTML, mimeHTML);
  });

  server->on("/control", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(controlHTML, mimeHTML);

    SwitchCommand command;
    command.freq = server->arg("freq").toFloat();
    command.pulse = server->arg("pulse").toInt();
    command.decimal = server->arg("decimal").toInt();
    command.bits = server->arg("bits").toInt();

    switchCommandQueue.enqueue(command);
  });

  server->on("/disconnect", HTTPMethod::HTTP_GET, [server](){
    configManager.clearWifiSettings(false);
  });

  server->on("/reboot", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(controlHTML, mimeHTML);
    ESP.restart();
  });

  server->on("/reset", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(resetHTML, mimeHTML);
    Log.notice(F("" CR));
    Log.notice(F("====================" CR));
    configManager.clearSettings(false);
  });

  server->on("/wipe", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(resetHTML, mimeHTML);
    configManager.clearSettings(false);
    configManager.clearWifiSettings(false);
    ESP.restart();
  });
}


// RTL 
//
const int messageBufferLen = 512;
char messageBuffer[messageBufferLen];

void rtlInit() {
    Log.notice(F(" " CR));
    Log.notice(F("****** RTL setup begin ******" CR));
    Log.notice(F("Frequency: %F" CR), config.frequency);
    rf.initReceiver(config.receivePin, config.frequency);
    enableRx();
    rf.setCallback(rtl433Callback, messageBuffer, messageBufferLen);
    Log.notice(F("****** RTL setup complete ******" CR));
}

void enableRx() {
    disableTx(); 

    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
    ELECHOUSE_cc1101.SetRx(config.frequency);
    ELECHOUSE_cc1101.setMHZ(config.frequency);

    rf.enableReceiver();
    //mySwitch.enableReceive(config.receivePin);
    Log.notice(F("****** Rx Enabled Pin %s ******" CR), String(config.receivePin));
}

void disableRx() {
    rf.disableReceiver();
    //mySwitch.disableReceive();
    Log.notice(F("****** Rx Disabled ******" CR));
}

void rfListen() {
  return;
  if (mySwitch.available()) {
    char* decoded = decode(mySwitch.getReceivedValue(), mySwitch.getReceivedBitlength(), mySwitch.getReceivedDelay(), mySwitch.getReceivedRawdata(),mySwitch.getReceivedProtocol());
    mySwitch.resetAvailable();
    Log.notice(F("Decoded: %s" CR), decoded);
    messagePost("sensor", decoded);
  }
}

void rtl433Callback(char* message) {
    Log.notice(F("Received message: %s" CR), message);
    messagePost("sensor", message);
}

void enableTx(float freq) {
    disableRx();

    //ELECHOUSE_cc1101.Init();
    //ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
    //ELECHOUSE_cc1101.setMHZ(freq);
    //ELECHOUSE_cc1101.SetTx();

    mySwitch.enableTransmit(config.transmitPin);
    Log.notice(F("****** Tx Enabled Pin %s ******" CR), String(config.transmitPin));
}

void disableTx() {
    mySwitch.disableTransmit();
    Log.notice(F("****** Tx Disabled ******" CR));
}

void processCommands() {
  if (switchCommandQueue.itemCount() > 0) {
      while (switchCommandQueue.itemCount() > 0) {
        struct switchCommand command = switchCommandQueue.dequeue();
        switchTransmit(command);
      }
      enableRx();
  }
}

void switchTransmit(struct switchCommand command) {
    Log.notice(F("Sending:" CR));
    Log.notice(F("  freq: %F" CR), command.freq);
    Log.notice(F("  pulse: %d" CR), command.pulse);
    Log.notice(F("  decimal: %d" CR), command.decimal);
    Log.notice(F("  bits %d" CR), command.bits);

    enableTx(command.freq);

    mySwitch.setPulseLength(command.pulse);
    mySwitch.send(command.decimal, command.bits);
}

// Main
//
void setup() {
    Serial.begin(921600);
    Log.begin(logLevel, &Serial);

    configSetup();
    rtlInit();
}

void loop() {
    configManager.loop();
    rf.loop();
    rfListen();

    unsigned long currentMillis = millis();
    if (!configManager.wifiConnected() && (currentMillis - previousMillis >= interval)) {
      WiFi.disconnect();
      WiFi.reconnect();
      previousMillis = currentMillis;
      Log.notice(F("Wifi Reconnect" CR));
    } else {
      processCommands();
    }
}

void messagePost(String path, char* message) {
    HTTPClient http;
    WiFiClient client;

    String url = String(config.serverURL);
    url.trim();

    http.begin(client, url + path);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(message);
}