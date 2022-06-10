#include <ConfigManager.h>
#include <arduino-timer.h>

ConfigManager configManager;

Timer<1, millis, void *> timer;

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

  // Callbacks
  configManager.setAPCallback(APCallback);
  configManager.setAPICallback(APICallback);
  configManager.begin(config);
}

void setConfigDefaults() {
  bool requireSave = false;

  char firstChar = config.deviceName[0];
  if (firstChar == '\0' || config.deviceName == NULL || firstChar == '\xFF') {
    strncpy(config.deviceName, "fesp-muppet", DEVICENAMELEN);
    requireSave = true;
  }

  if (int(config.ledPin) < 0) {
    config.ledPin = 1;
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
    setConfigDefaults();
    printConfig();

   server->on("/relay", HTTPMethod::HTTP_GET, [server](){
     configManager.streamFile(relayHTML, mimeHTML);
     if (tolower(server->arg("state")[1]) == 'n') {
       toggleState(true);
     } else {
       toggleState(false);
     }
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

  server->on("/relay", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(relayHTML, mimeHTML);
    if (tolower(server->arg("state")[1]) == 'n') {
      toggleState(true);
    } else {
      toggleState(false);
    }
  });

  setConfigDefaults();
  printConfig();
  led(HIGH);
}

void serveAssets(WebServer *server) {
  server->on("/styles.css", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(stylesCSS, mimeCSS);
  });

  server->on("/main.js", HTTPMethod::HTTP_GET, [server](){
    configManager.streamFile(mainJS, mimeJS);
  });
}

// Relay
//
void toggleState(bool state) {
  if (state) {
    relayOn();

    if (config.inchingDelay > 0) {
      timer.in(config.inchingDelay, timerCallback);
    }
  } else {
    relayOff();
  }
}

bool timerCallback(void *) {
  relayOff();
  debug("Timer Called", true);
  return false;
}

void relay(const byte *state) {
  pinMode(config.ledPin, INPUT);
  Serial.begin(9600);
  Serial.write(state, sizeof(state));
  Serial.flush();
  Serial.end();
}

void relayOn() {
  relay(relON);
  led(LOW);
  meta.isTriggered = true;
}

void relayOff() {
  relay(relOFF);
  led(HIGH);
  meta.isTriggered = false;
}

// LED
//
void flash() {
  int state = digitalRead(config.ledPin) == HIGH ? LOW : HIGH;
  led(state);
}

void led(int state) {
  pinMode(config.ledPin, OUTPUT);
  digitalWrite(config.ledPin, state);
}

// Main
//
void setup() {
  relayOff();
  delay(100);
  relayOff();

  configSetup();

  led(LOW);
  delay(2000);
  led(HIGH);
}

void loop() {
  configManager.loop();
  timer.tick();
}
