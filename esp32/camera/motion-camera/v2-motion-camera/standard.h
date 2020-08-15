#include "constants.h"

/* == Timer ==*/
#include <arduino-timer.h>
Timer<1, millis, void *> timer;

/* == SD ==*/
//#include "SD_MMC.h"
#include "SD.h"
bool sdEnabled = false;

/* == eMail ==*/
#include "ESP32_MailClient.h"

/* == ConfigManager ==*/
#include "configmanager.h"


/* == NTP ==*/
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
time_t bootTime = 0;

void socketLogger(String msg);
template<typename T>
void logger(T msg, bool newline = false);
template<typename T>
void loggerln(T msg);

void bootNotify() {
    if (!wifiConnected) return;

    Serial.println("Sending Boot Notification");
    SMTPData smtp;
    smtp.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);
    smtp.setSender("ESP32", emailSenderAccount);
    smtp.addRecipient(emailAlertAddress);
    smtp.setPriority("High");
    smtp.setSubject("Device Boot: " + (String) config.deviceName);
    smtp.setMessage("<div style=\"color:#2f4468;\"><h1>Hello World!</h1><p>- Sent from ESP32 board</p></div>", true);

    if (MailClient.sendMail(smtp)) {
        Serial.println("Boot Notification Sent");
    } else Serial.println("Error sending Email, " + MailClient.smtpErrorReason());

    smtp.empty();
}

void setTime() {
    unsigned long nowLong;
    timeval tv;

    timeClient.update();
    nowLong = timeClient.getEpochTime();
    tv.tv_sec=nowLong;
    tv.tv_usec = 0;
    settimeofday(&tv, 0);

    bootTime = time(NULL);
}

bool initSD() {
    if (sdEnabled) return sdEnabled;

    SPI.begin(14, 2, 15, 13);
    if(!SD.begin(13) || SD.cardType() == CARD_NONE) {
        Serial.println("SD Card Mount Failed");
        return false;
    }

    Serial.println("SD Card Detected.");
    sdEnabled = true;
    return sdEnabled;
}

bool closeSD() {
    SD.end();
    SPI.end();
    sdEnabled = false;
}

String saveFile(unsigned char* buf, unsigned int len, String path) {
    if (sdEnabled) {
        fs::FS &fs = SD;
        Serial.printf("Picture file name: %s\n", path.c_str());

        File handle = fs.open(path.c_str(), FILE_WRITE);
        if(!handle) {
            Serial.println("Failed to open file in writing mode");
            path = "";
        } else {
            handle.write(buf, len);
            Serial.printf("Saved file to path: %s\n", path.c_str());
            handle.close();
        }
    } else {
        loggerln("No SD Card Attached/Initialized");
        path = "";
    }

    return path;
}

bool rebootCallback(void *) {
    ESP.restart();
    loggerln('Rebooting...!');
    return false;
}

/*
void initHTTP(int port=80) {
    webServer.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
        timer.in(2000, rebootCallback);
        request->send(200, "text/plain", "Rebooting...");
    });

    webServer.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
        loggerln("/reset");
        configManager.clearAllSettings(false);
        timer.in(2000, rebootCallback);
        request->send(200, "text/plain", "Configs Cleared");
    });

    webServer.on("/reconfig", HTTP_GET, [](AsyncWebServerRequest *request) {
        loggerln("/reconfig");
        configDefaults();
        timer.in(2000, rebootCallback);
        request->send(200, "text/plain", "Configs Set to Defaults");
    });

    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        loggerln("/");
        request->send(SPIFFS, "/index.html", "text/html");
    });

    webServer.addHandler(&logSocket);
    webServer.begin();

    logger("Ready at: http://");
    logger(deviceIP());
    logger(":");
    loggerln(port);

    SPIFFS.begin();
}
*/


template<typename T>
void logger(T msg, bool newline) {
    bool debug = DEBUG_MODE;
    DEBUG_MODE = true;
    DebugPrint(msg);
    if (newline) DebugPrintln(F(""));
    Serial.flush();
    DEBUG_MODE = debug;
}

template<typename T>
void loggerln(T msg) {
    logger(msg, true);
}


