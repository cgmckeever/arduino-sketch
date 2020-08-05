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
} config;

void configSetup() {
    configManager.addParameter("deviceName", config.deviceName, DEVICENAMELEN);

    configManager.setAPCallback(APCallback);

    //disable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    configManager.begin(config);
    wifiConnected = configManager.getMode() == station;
    //enable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);
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