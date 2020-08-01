// ConfigManager
// 
#include <ConfigManager.h>
ConfigManager configManager;

const char *settingsHTML = (char *)"/settings.html";
const char *relayHTML = (char *)"/relay.html";
const char *resetHTML = (char *)"/reset.html";
const char *stylesCSS = (char *)"/styles.css";
const char *mainJS = (char *)"/main.js";

const int DEVICENAMELEN = 32;

struct Config {
  char deviceName[DEVICENAMELEN];
} config;

void configSetup() {
  String sApName = "SPYCAM";

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

void APCallback(WebServer *server) {
    serveAssets(server);
}


void APICallback(WebServer *server) {
  serveAssets(server);
}


void serveAssets(WebServer *server) {
  server->on("/styles.css", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(stylesCSS, mimeCSS);
  });
  
  server->on("/main.js", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(mainJS, mimeJS);
  });
}