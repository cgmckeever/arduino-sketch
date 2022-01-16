#include <ConfigManager.h>
#include <arduino-timer.h>

ConfigManager configManager;
Timer<1, millis, void *> timer0;
Timer<1, millis, void *> timer1;
Timer<1, millis, void *> timer2;
Timer<1, millis, void *> timer3;

const char *settingsHTML = (char *)"/settings.html";
const char *relayHTML = (char *)"/relay.html";
const char *resetHTML = (char *)"/reset.html";
const char *stylesCSS = (char *)"/styles.css";
const char *mainJS = (char *)"/main.js";

const int DEVICENAMELEN = 32;

struct Config {
  char deviceName[DEVICENAMELEN];
  float inchingDelay;
  int8_t ledPin;
} config;

struct Metadata {
  bool isTriggered;
  int8_t flashCount;
} meta;

// Hex relay commands
const byte relON[] = {0xA0, 0x01, 0x01, 0xA2};
const byte relOFF[] = {0xA0, 0x01, 0x00, 0xA1};

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
  String sApName = "ESPRELAY";

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

  config.ledPin = LED_BUILTIN;
  requireSave = true;

  char firstChar = config.deviceName[0];
  if (firstChar == '\0' || config.deviceName == NULL || firstChar == '\xFF') {
    strncpy(config.deviceName, "fesp-muppet", DEVICENAMELEN);
    requireSave = true;
  }

  if (float(config.inchingDelay) < 0 || isnan(config.inchingDelay)) {
    config.inchingDelay = 1000;
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
    timer0.every(10000, startFlashTask);
  } else {
    stopFlash();
  }
}

bool startFlashTask(void *) {
    return startFlash();
}

bool startFlash() {
  meta.isTriggered = true;
  ++meta.flashCount;
  debug("Flash Count: ");
  debug(meta.flashCount, true);

  timer1.in(1000, flashTimer1);
  timer2.in(4000, flashTimer2);
  timer3.in(7000, flashTimer3);

  if (meta.flashCount > 2) {
    debug("Flash Stopping", true);
    meta.flashCount = 0;
    meta.isTriggered = false;
  }
  return meta.isTriggered;
}

bool flashTimer1(void *) {
  // call whatever relay to trigger on/off
  flashLed();
  debug("flash1", true);
  return false;
}

bool flashTimer2(void *) {
  // call whatever relay to trigger on/off
  flashLed();
  debug("flash2", true);
  return false;
}

bool flashTimer3(void *) {
  // call whatever relay to trigger on/off
  flashLed();
  debug("flash3", true);
  return false;
}

void stopFlash() {
  relayControl(relOFF);
  led(HIGH);
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
void relayControl(const byte *state) {
  pinMode(config.ledPin, INPUT);
  Serial.begin(9600);
  Serial.write(state, sizeof(state));
  Serial.flush();
  Serial.end();
}

void led(int state) {
  debug("LED", true);
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

  startFlash();
  timer0.every(10000, startFlashTask);
}

void loop() {
  configManager.loop();
  timer0.tick();
  timer1.tick();
  timer2.tick();
  timer3.tick();
}

