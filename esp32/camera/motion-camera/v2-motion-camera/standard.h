#include "constants.h"
#include <tuple>
#include <iostream>

extern String deviceName;

/* == SD ==*/
//#include "SD_MMC.h"
#include "SD.h"
bool sdEnabled = false;

/* == eMail ==*/
#include "ESP32_MailClient.h"

//const char *settingsHTML = (char *)"/settings.html";
//const char *resetHTML = (char *)"/reset.html";
const char *stylesCSS = (char *)"/styles.css";
const char *mainJS = (char *)"/main.js";

/* == ConfigManager ==*/
#include "configmanager.h"

/* == HTTP Server ==*/
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
extern AsyncWebServer webServer;

/* == NTP ==*/
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
time_t bootTime = 0;

/* == Timer ==*/
#include <arduino-timer.h>
Timer<1, millis, void *> timer;

void bootNotify() {
    if (!wifiConnected) return;

    SMTPData smtp;
    smtp.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);
    smtp.setSender("ESP32", emailSenderAccount);
    smtp.addRecipient(emailAlertAddress);
    smtp.setPriority("High");
    smtp.setSubject("Device Boot: " + deviceName);
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

String saveFile(unsigned char *buf, unsigned int len, String path) {
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
        Serial.println("No SD Card Attached/Initialized");
        path = "";
    }

    return path;
}

bool rebootCallback(void *) {
    ESP.restart();
    Serial.println('Rebooting...');
    return false;
}

void initHTTP(int port=80) {
    webServer.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
        timer.in(2000, rebootCallback);
        request->send(200, "text/plain", "Rebooting...");
    });

    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        timer.in(2000, rebootCallback);
        request->send(200, "text/plain", "Hello world!");
    });

    webServer.begin();

    Serial.print("Ready at: http://");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.println(port);
}
