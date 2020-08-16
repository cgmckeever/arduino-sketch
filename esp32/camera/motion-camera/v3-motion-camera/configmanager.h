/* == ConfigManager ==*/
#include <ConfigManager.h>
ConfigManager configManager;
bool wifiConnected = false;

void apModeCallback(WebServer* server);
void stationModeCallback(WebServer* server);

/* == Brownout Handler ==*/
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

//const char *settingsHTML = (char *) "/settings.html";
//const char *resetHTML = (char *) "/reset.html";
const char *stylesCSS = (char *) "/styles-ap.css";
const char *mainJS = (char *) "/main-ap.js";

char *deviceName = "Spy-Cam-v2";
const int DEVICENAMELEN = 28;
const int APPASSLEN = 15;

struct Config {
    char deviceName[DEVICENAMELEN];
    char apPassword[APPASSLEN];
    int captureFramesize;
    bool disableCameraMotion;
    bool sendAlerts;

    int streamFramesize;
    int streamQueueMax;
    int streamSizeQueued;
    int streamWaitMin;

    int camera_xclk_freq_hz;
    int camera_exposure;
    int camera_exposure_control;
    int camera_hmirror;
    int camera_vflip;
    int camera_lenc;
    int camera_raw_gma;
    // BOOT TS?
} config;

bool stringEmpty(char* checkString) {
  char firstChar = checkString[0];
  return (firstChar == '\0' || checkString == NULL || firstChar == '\xFF');
}

void configDefaults() {
  Serial.println("config defaults set");

  strncpy(config.deviceName, "Spy-Cam-v2", DEVICENAMELEN);
  strncpy(config.apPassword, "1234567890", APPASSLEN);

  config.captureFramesize = 9;
  config.disableCameraMotion = true;
  config.sendAlerts = true;

  config.streamFramesize = 3;

  config.camera_xclk_freq_hz = 20000000;
  config.camera_exposure = 600;
  config.camera_exposure_control = 0;
  config.camera_hmirror = 0;
  config.camera_vflip = 0;
  config.camera_lenc = 0;
  config.camera_raw_gma = 0;

  configManager.save();
}

void configSetup() {
    configManager.addParameter("deviceName", config.deviceName, DEVICENAMELEN);
    configManager.addParameter("captureFramesize", &config.captureFramesize);
    configManager.addParameter("disableDeviceMotion", &config.disableCameraMotion);
    configManager.addParameter("sendAlerts", &config.sendAlerts);

    configManager.addParameter("streamFramesize", &config.streamFramesize);

    // Camera Settings
    configManager.addParameter("cameraFreq", &config.camera_xclk_freq_hz);
    configManager.addParameter("cameraExposure", &config.camera_exposure);
    configManager.addParameter("cameraExposureControl", &config.camera_exposure_control);
    configManager.addParameter("cameraHmirror", &config.camera_hmirror);
    configManager.addParameter("cameraVflip", &config.camera_vflip);
    configManager.addParameter("cameraLensCorrection", &config.camera_lenc);
    configManager.addParameter("cameraRawGMA", &config.camera_raw_gma);

    configManager.setInitCallback(configDefaults);
    configManager.setAPCallback(stationModeCallback);
    configManager.setAPICallback(stationModeCallback);
    configManager.setAPFilename("/wifiConfig.html");

    configManager.setAPName("Spy-Cam-v2");

    //disable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    configManager.begin(config);
    wifiConnected = configManager.getMode() == station;
    //enable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);
}

IPAddress deviceIP() {
  return wifiConnected ? WiFi.localIP() : WiFi.softAPIP();
}

void serveAssets(WebServer *server) {
  server->on("/styles-ap.css", HTTPMethod::HTTP_GET, [server]() {
    configManager.streamFile(stylesCSS, mimeCSS);
  });

  server->on("/main-ap.js", HTTPMethod::HTTP_GET, [server]() {
    configManager.streamFile(mainJS, mimeJS);
  });

  Serial.println("Assets Registered");
}