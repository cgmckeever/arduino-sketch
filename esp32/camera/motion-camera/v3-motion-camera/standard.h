#include "constants.h"

template<typename T>
void logger(T msg, bool newline = false);
template<typename T>
void loggerln(T msg);

/* == Timer ==*/
//#include <arduino-timer.h>
//Timer<1, millis, void *> timer;

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

bool requireReboot = false;

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
        loggerln("Boot Notification Sent");
    } else loggerln("Error sending Email, " + MailClient.smtpErrorReason());

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
        loggerln("SD Card Mount Failed");
        return false;
    }

    loggerln("SD Card Detected.");
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
        logger("Picture file name: ");
        loggerln(path.c_str());

        File handle = fs.open(path.c_str(), FILE_WRITE);
        if(!handle) {
            loggerln("Failed to open file in writing mode");
            path = "";
        } else {
            handle.write(buf, len);
            logger("Saved file to path: ");
            loggerln(path.c_str());
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

void initHTTP(WebServer* server) {

    server->on("/reboot", HTTPMethod::HTTP_GET, [server]() {
        requireReboot = true;
        server->send(200, "text/plain", "Rebooting...");
    });

    server->on("/reset", HTTPMethod::HTTP_GET, [server]() {
        loggerln("/reset");
        configManager.clearAllSettings(false);
        requireReboot = true;
        server->send(200, "text/plain", "Configs Cleared");
    });

    server->on("/reconfig", HTTPMethod::HTTP_GET, [server]() {
        loggerln("/reconfig");
        configDefaults();
        requireReboot = true;
        server->send(200, "text/plain", "Configs Set to Defaults");
    });

    server->on("/", HTTPMethod::HTTP_GET, [server]() {
        loggerln("/");
        configManager.streamFile("/index.html", mimeHTML);
    });
}


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


