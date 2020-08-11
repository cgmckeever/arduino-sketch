/* == ConfigManager ==*/
#include <ConfigManager.h>
ConfigManager configManager;
bool wifiConnected = false;
void APCallback(WebServer *server);

/* == Brownout Handler ==*/
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

const int DEVICENAMELEN = 28;
const int APPASSLEN = 15;

struct Config {
    char deviceName[DEVICENAMELEN];
    char apPassword[APPASSLEN];
    int captureFramesize;
    int streamFramesize;
    int streamQueue;
    bool disableCameraMotion;
    // BOOT TS?
} config;

void configSave() {
  deviceName = config.deviceName;
  configManager.save();
}

bool stringEmpty(char* checkString) {
  char firstChar = checkString[0];
  return (firstChar == '\0' || checkString == NULL || firstChar == '\xFF');
}

void configDefaults() {
  if (stringEmpty(config.deviceName)) {
    strncpy(config.deviceName, "Spy-Cam", DEVICENAMELEN);
  }

  if (stringEmpty(config.apPassword)) {
    strncpy(config.apPassword, "1234567890", APPASSLEN);
  }

  if (config.captureFramesize < 1) config.captureFramesize = 9;
  if (config.streamFramesize < 1 || config.streamFramesize > 6) config.streamFramesize = 5;
  if (config.streamQueue < 1) config.streamQueue = 2;

  configSave();
}

void configSetup() {
    configManager.addParameter("deviceName", config.deviceName, DEVICENAMELEN);
    configManager.addParameter("captureFramesize", &config.captureFramesize);
    configManager.addParameter("streamFramesize", &config.streamFramesize);
    configManager.addParameter("streamQueue", &config.streamQueue);
    configManager.addParameter("disableDeviceMotion", &config.disableCameraMotion);

    configManager.setAPCallback(APCallback);
    configManager.setAPFilename("index.ap.html");

    configManager.setAPName(deviceName);

    //disable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    configManager.begin(config);
    configDefaults();
    wifiConnected = configManager.getMode() == station;
    //enable brownout detector

    if (!wifiConnected) {
      WiFi.softAPdisconnect();
      configManager.setAPName(config.deviceName);
      configManager.setAPPassword(config.apPassword);
      configManager.startAP();
      Serial.println(config.deviceName);
    }
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);

}

IPAddress deviceIP() {
  return wifiConnected ? WiFi.localIP() : WiFi.softAPIP();
}

void serveAssets(WebServer *server) {
  server->on("/styles.ap.css", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(stylesCSS, mimeCSS);
  });

  server->on("/main.ap.js", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(mainJS, mimeJS);
  });
}

void APCallback(WebServer *server) {
    serveAssets(server);
}