#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <fauxmoESP.h>
#include <ConfigManager.h>
#include <Scheduler.h>

fauxmoESP fauxmo;
ConfigManager configManager;

// set to false to test BLINK toggle via relay mock
#define relayMode false
#define toggleON() (relayMode ? relay(relON) : led(true))
#define toggleOFF() (relayMode ? relay(relOFF) : led(false))

const char *settingsHTML = (char *)"/settings.html";
const char *controlHTML = (char *)"/control.html";
const char *stylesCSS = (char *)"/styles.css";
const char *mainJS = (char *)"/main.js";

const int DEVICENAMELEN = 32;

struct Config {
  char deviceName[DEVICENAMELEN];
  float inchingDelay;
  int8_t led;
} config;

struct Metadata {
  bool triggered;
} meta;

// Hex command to send to serial for open relay
const byte relON[] = {0xA0, 0x01, 0x01, 0xA2};  

// Hex command to send to serial for close relay
const byte relOFF[] = {0xA0, 0x01, 0x00, 0xA1}; 

template<typename T>
void debug(T &msg, bool newline = false) {
  DebugPrint(msg);
  if (newline) DebugPrintln(F(""));
}

void APCallback(WebServer *server) {
    server->on("/styles.css", HTTPMethod::HTTP_GET, [server](){
        configManager.streamFile(stylesCSS, mimeCSS);
    });

    setConfigDefaults();
    printConfig();
}

void APICallback(WebServer *server) {
  server->on("/disconnect", HTTPMethod::HTTP_GET, [server](){
    configManager.clearWifiSettings(false);
  });
  
  server->on("/settings.html", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(settingsHTML, mimeHTML);
  });

  server->on("/control.html", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(controlHTML, mimeHTML);
    if (tolower(server->arg("state")[1]) == 'n') {
      toggleState(true);
    } else {
      toggleState(false);
    }
  });
  
  server->on("/styles.css", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(stylesCSS, mimeCSS);
  });
  
  server->on("/main.js", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(mainJS, mimeJS);
  });

  setConfigDefaults();
  printConfig();
  fauxmoConfig();
  led(false);
  
  if (relayMode) {
    serialMode();
    toggleOFF();
  }
}


void setConfigDefaults() {
  bool requireSave = false;

  char firstChar = config.deviceName[0];
  if (firstChar == '\0' || config.deviceName == NULL || firstChar == '\xFF') {
    strncpy(config.deviceName, "muppet", DEVICENAMELEN);
    requireSave = true;
  }

  if (int(config.led) < 0) {
    config.led = 2;
    requireSave = true;
  }

  if (float(config.inchingDelay) < 0 || isnan(config.inchingDelay)) {
    config.inchingDelay = 1;
    requireSave = true;
  }

  if (requireSave) configManager.save();
}

void printConfig() {
  debug("Configuration: ", true);
  debug(config.led, true);
  debug(config.inchingDelay, true);
  debug(config.deviceName, true);
}

void fauxmoConfig() {
  fauxmo.setPort(80);  
  fauxmo.enable(true);

  debug("Device discoverable as: ");
  debug(config.deviceName, true);
  fauxmo.addDevice(config.deviceName); 

  fauxmo.onSetState([](unsigned char deviceId, const char * deviceName, 
                    bool state, unsigned char value) {

    DebugPrint("State: ");
    DebugPrintln(state);
    DebugPrint("Value: ");
    DebugPrintln(value);

    toggleState(state);
 
  });

}

void toggleState(bool state) {
    if (state) {
      toggleON();
      meta.triggered = true;
    } else {
      meta.triggered = false;
      toggleOFF();
    }
}

void relay(const byte relState[]) {
  serialMode();
  Serial.write(relState, sizeof(relState));
  Serial.flush();
}

void led(bool is_on) {
  int LED = getLED();
  pinMode(LED, OUTPUT);
  digitalWrite(LED, is_on ? LOW : HIGH);
}

void serialMode() {
  pinMode(getLED(), INPUT);
}

int getLED() {
  // NodeMCU = 2 16; ESP-01 = 1;
  return config.led > 0 ? config.led : 2;
}

class FauxmoTask : public Task {
protected:
    void setup() {}

    void loop() {
        fauxmo.handle();
    }
} fauxmoTask;

class ConfigureTask : public Task {
protected:
    void setup() {}

    void loop() {
        configManager.loop();
    }
} configureTask;

class InchingTask : public Task {
protected:
    void setup() {}

    void loop() {
        if (meta.triggered && config.inchingDelay > 0) {
          delay(config.inchingDelay * 1000);
          meta.triggered = false;
          toggleOFF();
        }
    }
} inchingTask;

void setup() { 
  led(true);
  delay(1000);

  DEBUG_MODE = true;
  Serial.begin(9600);

  meta.triggered = false;
  
  randomSeed(*(volatile uint32_t *)0x3FF20E44);
  String sApName = "ESPRELAY"; // -" + String(random(111, 999));
  configManager.setAPName(sApName.c_str());
  configManager.setAPFilename("/index.html");
  configManager.setWebPort(8080);

  // Config
  configManager.addParameter("deviceName", config.deviceName, DEVICENAMELEN);
  configManager.addParameter("inchingDelay", &config.inchingDelay);
  configManager.addParameter("led", &config.led);

  // Meta
  configManager.addParameter("triggered", &meta.triggered, get);

  configManager.setAPCallback(APCallback);
  configManager.setAPICallback(APICallback);
  configManager.begin(config);

  Scheduler.start(&configureTask);
  Scheduler.start(&fauxmoTask);
  Scheduler.start(&inchingTask);
  Scheduler.begin();
}


void loop() {}
