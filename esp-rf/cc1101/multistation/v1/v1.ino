#include <ConfigManager.h>
ConfigManager configManager;

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

template<typename T>
void debug(T &msg, bool newline = false) {
  DEBUG_MODE = true;
  Serial.begin(115200);
  DebugPrint(msg);
  if (newline) DebugPrintln(F(""));
  Serial.flush();
  Serial.end();
}

// ConfigManager
//
void configSetup() {
  DEBUG_MODE = true;
  Serial.begin(115200);

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
  debug("Configuration: ");
  debug(config.deviceName, true);
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

// Main
//
void setup() {
  configSetup();
}

void loop() {
  configManager.loop();
}


