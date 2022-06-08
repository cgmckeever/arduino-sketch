#include <ConfigManager.h>
ConfigManager configManager;

#include <HTTPClient.h>

#include <ArduinoLog.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#include <rtl_433_ESP.h>
rtl_433_ESP rf(-1);

#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();

// params
//
const char *settingsHTML = (char *)"/settings.html";
const char *resetHTML = (char *)"/reset.html";
const char *stylesCSS = (char *)"/styles.css";
const char *mainJS = (char *)"/main.js";

const char *controlHTML = (char *)"/control.html";

const int DEVICENAMELEN = 32;

struct Config {
  char deviceName[DEVICENAMELEN];
} config;

struct Metadata {
} meta;


#define JSON_MSG_BUFFER 512
#define LOG_LEVEL LOG_LEVEL_VERBOSE

// these can become CONFIGS
#define CC1101_FREQUENCY 433.92
#define RF_RECEIVER_GPIO 27
#define RF_EMITTER_GPIO 26
const String NodeRed = "http://192.168.2.25:1880/";

char messageBuffer[JSON_MSG_BUFFER];

// ConfigManager
//
void configSetup() {
  DEBUG_MODE = true;

  String sApName = "ESP";

  configManager.setAPName(sApName.c_str());
  configManager.setAPFilename("/index.html");
  configManager.setWebPort(8080);

  // Config
  configManager.addParameter("deviceName", config.deviceName, DEVICENAMELEN);

  // Callbacks
  configManager.setAPCallback(APCallback);
  configManager.setAPICallback(APICallback);
  configManager.begin(config);
}

void setConfigDefaults() {
  bool requireSave = false;

  char firstChar = config.deviceName[0];
  if (firstChar == '\0' || config.deviceName == NULL || firstChar == '\xFF') {
    strncpy(config.deviceName, "rascal", DEVICENAMELEN);
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
  });

  setConfigDefaults();
  printConfig();
}

// RTL
//
void rtlSetup() {
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setMHZ(CC1101_FREQUENCY);
    ELECHOUSE_cc1101.SetRx(CC1101_FREQUENCY);

    Log.notice(F(" " CR));
    Log.notice(F("****** RTL setup begin ******" CR));
    rf.initReceiver(RF_RECEIVER_GPIO, CC1101_FREQUENCY);
    rf.setCallback(rtl433Callback, messageBuffer, JSON_MSG_BUFFER);
    rf.enableReceiver(RF_RECEIVER_GPIO);
    Log.notice(F("****** RTL setup complete ******" CR));
}

void myswitchSetup() {
    rf.disableReceiver();
    mySwitch.enableTransmit(RF_EMITTER_GPIO);
}

void rtl433Callback(char* message) {
    Log.notice(F("Received message: %s" CR), message);
    messagePost("sensor", message);

    JsonObject obj = sensorObj(message);
    String model = obj["model"];
    Log.notice(F("Model: %s" CR), model);
}

void messagePost(String path, char* message) {
    HTTPClient http;
    WiFiClient client;

    http.begin(client, NodeRed + "/" + path);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(message);
}

JsonObject sensorObj(char* message) {
    DynamicJsonDocument doc(JSON_MSG_BUFFER);

    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        Log.error(F("deserializeJson() failed: "));
        Log.errorln(error.f_str());
    }

    return doc.as<JsonObject>();
}

// Main
//
void setup() {
    Serial.begin(115200);
    Log.begin(LOG_LEVEL, &Serial);

    configSetup();
    rtlSetup();
}

void loop() {
    configManager.loop();
    rf.loop();
}


