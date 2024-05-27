#include <ArduinoLog.h>
#include <Arduino.h>

#include <ConfigManager.h>
ConfigManager configManager;

#include <HTTPClient.h>


#include <main.h>

#define logLevel LOG_LEVEL_VERBOSE

// ConfigManger
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
}

void setConfigDefaults() {
  bool requireSave = false;

  char firstChar = config.deviceName[0];
  if (firstChar == '\0' || config.deviceName == NULL || firstChar == '\xFF') {
    strncpy(config.deviceName, "device", deviceNameLen);
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

    //SwitchCommand command;
    //command.freq = server->arg("freq").toFloat();
    //command.pulse = server->arg("pulse").toInt();
    //command.decimal = server->arg("decimal").toInt();
    //command.bits = server->arg("bits").toInt();

    //switchCommandQueue.enqueue(command);
  });

  setConfigDefaults();
  printConfig();
}


// RTL 

void setup() {
    Serial.begin(115200);
    Log.begin(logLevel, &Serial);

    configSetup();
    //rtlSetup();
}

void loop() {
    configManager.loop();
    //rf.loop();
    
    unsigned long currentMillis = millis();
    if (!configManager.wifiConnected() && (currentMillis - previousMillis >= interval)) {
      WiFi.disconnect();
      WiFi.reconnect();
      previousMillis = currentMillis;
      Log.notice(F("Wifi Reconnect" CR));
    } else {
      //processCommands();
    }
}
