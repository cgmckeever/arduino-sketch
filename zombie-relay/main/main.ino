#include <ConfigManager.h>
#include <arduino-timer.h>

// Hex relay commands
const byte rel1OFF[] = {0xA0,0x01,0x00,0xA1};
const byte rel1ON[] = {0xA0,0x01,0x01,0xA2};

const byte rel2OFF[] = {0xA0,0x02,0x00,0xA2};
const byte rel2ON[] = {0xA0,0x02,0x01,0xA3};

const byte rel3OFF[] = {0xA0,0x03,0x00,0xA3};
const byte rel3ON[] = {0xA0,0x03,0x01,0xA4};

const byte rel4OFF[] = {0xA0,0x04,0x00,0xA4};
const byte rel4ON[] = {0xA0,0x04,0x01,0xA5};

const bool ON = true;
const bool OFF = false;

const int delayOffset = 3000;
int relayDelay;

ConfigManager configManager;
Timer<1, millis, void *> timer0;
Timer<6, millis, int8_t> timer1;

const char *settingsHTML = (char *)"/settings.html";
const char *relayHTML = (char *)"/relay.html";
const char *resetHTML = (char *)"/reset.html";
const char *stylesCSS = (char *)"/styles.css";
const char *mainJS = (char *)"/main.js";

const int DEVICENAMELEN = 32;

// ESP-01 == 1
// ESP-01s == 2 (LED_BUILTIN)
const int ledPin = LED_BUILTIN;

const int relayCount = 3;
int relayBaud;

bool demo = true;

struct Config {
  char deviceName[DEVICENAMELEN];
  float inchingDelay;
  int8_t ledPin;
} config;

struct Metadata {
  bool isTriggered;
  int8_t flashCount;
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

  // randomSeed(*(volatile uint32_t *)0x3FF20E44);
  // String sApName = "ESPRELAY-" + String(random(111, 999));
  String sApName = "ZOMBIEFLASH1";

  configManager.setAPName(sApName.c_str());
  configManager.setAPFilename("/index.html");
  configManager.setWebPort(8080);

  // Config
  configManager.addParameter("deviceName", config.deviceName, DEVICENAMELEN);
  configManager.addParameter("inchingDelay", &config.inchingDelay);
  configManager.addParameter("ledPin", &config.ledPin);
  configManager.addParameter("isTriggered", &meta.isTriggered, get);
  configManager.addParameter("flashCount", &meta.flashCount, get);

  // Callbacks
  configManager.setAPCallback(APCallback);
  configManager.setAPICallback(APICallback);
  configManager.begin(config);

  setConfigDefaults();
  printConfig();
}

void setConfigDefaults() {
  bool requireSave = false;

  meta.flashCount = 0;
  meta.isTriggered = false;

  char firstChar = config.deviceName[0];
  if (firstChar == '\0' || config.deviceName == NULL || firstChar == '\xFF') {
    strncpy(config.deviceName, "fesp-muppet", DEVICENAMELEN);
    requireSave = true;
  }

  if (float(config.inchingDelay) < 0 || isnan(config.inchingDelay)) {
    config.inchingDelay = 1000;
    requireSave = true;
  }

  if (config.ledPin != ledPin) {
    config.ledPin = ledPin;
    requireSave = true;
  }

  if (requireSave) configManager.save();
}

void printConfig() {
  debug("Configuration: ");
  debug(config.deviceName, true);

  debug("LED Pin: ");
  debug(config.ledPin, true);

  debug("Inching Delay: ");
  debug(config.inchingDelay, true);
}

void APCallback(WebServer *server) {
    serveAssets(server);

   server->on("/control", HTTPMethod::HTTP_GET, [server](){
     configManager.streamFile(relayHTML, mimeHTML);

     bool trigger = false;
     if (tolower(server->arg("state")[1]) == 'n') {
       trigger = true;
     }
     toggleState(trigger);
   });
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
    configManager.streamFile(relayHTML, mimeHTML);

    bool trigger = false;
    if (tolower(server->arg("state")[1]) == 'n') {
      trigger = true;
    }
    toggleState(trigger);
  });
}

void serveAssets(WebServer *server) {
  server->on("/styles.css", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(stylesCSS, mimeCSS);
  });

  server->on("/main.js", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(mainJS, mimeJS);
  });
}

// Flash Logic
//
void toggleState(bool state) {
  if (state) {
    startFlash();
    timer0.every(relayDelay, flashTask);
  } else {
    stopFlash();
  }
}

bool flashTask(void *) {
  startFlash();
  if (meta.flashCount > 2) stopFlash();
  return meta.isTriggered;
}

void startFlash() {
  meta.isTriggered = true;
  ++meta.flashCount;
  debug("Flash Count: ");
  debug(meta.flashCount, true);

  int delay = 1000;
  for (int i = 1; i <= relayCount; i++) {
    timer1.in(delay, flashTimer, i);
    delay += delayOffset;
  }
}

bool flashTimer(int8_t relayNum) {
  relayControl(relayNum, ON);
  timer1.in(500, flashOff, relayNum);
  debug("Flash: ");
  debug(relayNum, true);
  return false;
}

bool flashOff(int8_t relayNum) {
  relayControl(relayNum, OFF);
  debug("Off: ");
  debug(relayNum, true);
  return false;
}

void stopFlash() {
  debug("Flash Stopping", true);
  meta.flashCount = 0;
  meta.isTriggered = false;
  timer0.cancel();
}

void flashLed() {
  int state = digitalRead(config.ledPin) == HIGH ? LOW : HIGH;
  led(state);
}

// Controls
//
const byte* relayCommand(int8_t relayNum, bool on) {
    switch(relayNum) {
        case 1:
          return on ? rel1ON : rel1OFF;
        case 2:
          return on ? rel2ON : rel2OFF;
        case 3:
          return on ? rel3ON : rel3OFF;
        case 4:
          return on ? rel4ON : rel4OFF;
    }
}

void relayControl(int8_t relayNum, bool state) {
  Serial.begin(relayBaud);
  Serial.write(relayCommand(relayNum, state), sizeof(relayCommand(relayNum, state)));
  Serial.flush();
  Serial.end();
}

void led(int state) {
  pinMode(config.ledPin, OUTPUT);
  digitalWrite(config.ledPin, state);
}

// Main
//
void setup() {
  configSetup();

  flashLed();
  delay(1000);
  flashLed();

  switch(relayCount) {
    case 1:
      relayBaud = 9600;
    case 2:
    case 3:
    case 4:
      relayBaud = 115200;
  }

  relayDelay = delayOffset * relayCount + 1000;

  if (demo) {
    for (int i = 1; i <= relayCount; i++) {
      relayControl(i, ON);
      delay(1500);
      relayControl(i, OFF);
      delay(500);
    }

    startFlash();
    timer0.every(relayDelay, flashTask);
  }
}

void loop() {
  configManager.loop();
  timer0.tick();
  timer1.tick();
}

