/* == ConfigManager ==*/
#include <ConfigManager.h>
ConfigManager configManager;
bool wifiConnected = false;
void APCallback(WebServer *server);

/* == Brownout Handler ==*/
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

const int DEVICENAMELEN = 28;

struct Config {
    char deviceName[DEVICENAMELEN];
    int captureFramesize;
    int streamFramesize;
    int streamQueue;
    bool disableCameraMotion;
} config;

void configDefaults() {
  char firstChar = config.deviceName[0];
  if (firstChar == '\0' || config.deviceName == NULL || firstChar == '\xFF') {
    strncpy(config.deviceName, "Spy Cam", DEVICENAMELEN);
  }

  if (config.captureFramesize < 1) config.captureFramesize = 9;
  if (config.streamFramesize < 1 || config.streamFramesize > 6) config.streamFramesize = 5;
  if (config.streamQueue < 1) config.streamQueue = 2;

  configManager.save();
}

void configSetup() {
    configManager.addParameter("deviceName", config.deviceName, DEVICENAMELEN);
    configManager.addParameter("captureFramesize", &config.captureFramesize);
    configManager.addParameter("streamFramesize", &config.streamFramesize);
    configManager.addParameter("streamQueue", &config.streamQueue);
    configManager.addParameter("disableDeviceMotion", &config.disableCameraMotion);

    configManager.setAPCallback(APCallback);
    configManager.setAPFilename("index.ap.html");

    //disable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    configManager.begin(config);
    wifiConnected = configManager.getMode() == station;
    //enable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);

    configDefaults();
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