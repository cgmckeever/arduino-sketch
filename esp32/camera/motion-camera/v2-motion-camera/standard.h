#include "constants.h"
#include <WiFi.h>


// Brownout Handler
//
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// NTP
//
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#include "SD.h" 
bool sdEnabled = false;

#include "ESP32_MailClient.h"
SMTPData smtpMotion;


void startWifi() {
    //disable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

    Serial.println("WiFi connecting..");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");

    //enable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1); 
}

void setTime() {
    unsigned long nowLong;
    timeval tv;

    timeClient.update();
    nowLong = timeClient.getEpochTime();
    tv.tv_sec=nowLong;
    tv.tv_usec = 0;
    settimeofday(&tv, 0);
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
    return true;
}

bool closeSD() {
    SD.end();
    SPI.end();
    sdEnabled = false;
}

String saveFile(unsigned char *buf, unsigned int len, String path) {
    if (initSD()) {
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
        closeSD();
    } else {
        Serial.println("No SD Card Attached");
        path = "";
    }

    return path;
}