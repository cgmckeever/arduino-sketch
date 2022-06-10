#include <ConfigManager.h>
ConfigManager configManager;

#include <HTTPClient.h>

#include <ArduinoLog.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#include <rtl_433_ESP.h>
rtl_433_ESP rf(-1);

#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();

#define logLevel LOG_LEVEL_VERBOSE

// params
//
const char *settingsHTML = (char *)"/settings.html";
const char *resetHTML = (char *)"/reset.html";
const char *stylesCSS = (char *)"/styles.css";
const char *mainJS = (char *)"/main.js";

const char *controlHTML = (char *)"/control.html";

const int deviceNameLen = 32;
const int messageBufferLen = 512;
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

char messageBuffer[messageBufferLen];

// ConfigManager
//
void configSetup() {
  DEBUG_MODE = true;

  String sApName = "ESP";

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
}

void setConfigDefaults() {
  bool requireSave = false;

  char firstChar = config.deviceName[0];
  if (firstChar == '\0' || config.deviceName == NULL || firstChar == '\xFF') {
    strncpy(config.deviceName, "rascal", deviceNameLen);
    requireSave = true;
  }

  firstChar = config.serverURL[0];
  if (firstChar == '\0' || config.serverURL == NULL || firstChar == '\xFF') {
    strncpy(config.serverURL, "http://192.168.2.25:1880/", serverURLLen);
    requireSave = true;
  }

  if (float(config.frequency) < 0 || isnan(config.frequency)) {
    config.frequency = 433.92;
    requireSave = true;
  }

  if (int(config.receivePin) < 0) {
    config.receivePin = 27;
    requireSave = true;
  }

  if (int(config.transmitPin) < 0) {
    config.transmitPin = 26;
    requireSave = true;
  }

  if (requireSave) configManager.save();
}

void printConfig() {
  Log.notice(F("Configuration" CR));
  Log.notice(F("Device Name : %s" CR), config.deviceName);
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
    setConfigDefaults();
    printConfig();
}

void APICallback(WebServer *server) {
  serveAssets(server);

  server->on("/disconnect", HTTPMethod::HTTP_GET, [server](){
    configManager.clearWifiSettings(false);
  });

  server->on("/reset", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(resetHTML, mimeHTML);
    configManager.clearSettings(false);
  });

  server->on("/wipe", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(resetHTML, mimeHTML);
    configManager.clearWifiSettings(false);
    configManager.clearSettings(false);
    ESP.restart();
  });

  server->on("/config", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(settingsHTML, mimeHTML);
  });

  server->on("/control", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(controlHTML, mimeHTML);

    float freq = server->arg("freq").toFloat();
    int pulse = server->arg("pulse").toInt();
    int decimal = server->arg("decimal").toInt();
    int bits = server->arg("bits").toInt();

    switchTransmit(freq, pulse, decimal, bits);
  });

  setConfigDefaults();
  printConfig();
}

// RTL
//
void rtlSetup() {
    Log.notice(F(" " CR));
    Log.notice(F("****** RTL setup begin ******" CR));
    Log.notice(F("Frequency: %F" CR), config.frequency);
    rf.initReceiver(config.receivePin, config.frequency);
    enableRx();
    Log.notice(F("****** RTL setup complete ******" CR));
}

void enableRx() {
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
    ELECHOUSE_cc1101.SetRx(config.frequency);
    ELECHOUSE_cc1101.setMHZ(config.frequency);

    rf.setCallback(rtl433Callback, messageBuffer, messageBufferLen);
    rf.enableReceiver(config.receivePin);
}

void disableRX() {
    rf.enableReceiver(-1);
    rf.disableReceiver();
}

void switchTransmit(float freq, int pulse, int decimal, int bits) {
    Log.notice(F("Sending:" CR));
    Log.notice(F("  freq: %F" CR), freq);
    Log.notice(F("  pulse: %d" CR), pulse);
    Log.notice(F("  decimal: %d" CR), decimal);
    Log.notice(F("  bits %d" CR), bits);

    disableRX();

    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
    ELECHOUSE_cc1101.setMHZ(freq);
    ELECHOUSE_cc1101.SetTx();

    mySwitch.enableTransmit(config.transmitPin);
    mySwitch.setPulseLength(pulse);
    mySwitch.send(decimal, bits);
    mySwitch.disableTransmit();

    enableRx();
}

void rtl433Callback(char* message) {
    Log.notice(F("Received message: %s" CR), message);
    messagePost("sensor", message);
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

// Main
//
void setup() {
    Serial.begin(115200);
    Log.begin(logLevel, &Serial);

    configSetup();
    rtlSetup();
}

void loop() {
    configManager.loop();
    rf.loop();
}

