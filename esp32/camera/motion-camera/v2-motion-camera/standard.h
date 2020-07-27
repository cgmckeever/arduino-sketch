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

// SD
//
#include "SD.h" 
bool sdEnabled = false;

// eMail
//
#include "ESP32_MailClient.h"
SMTPData smtpMotion;

// HTTP Server
//
#include "esp_http_server.h"
httpd_handle_t server = NULL;

// Timer
//
#include <arduino-timer.h>
Timer<1, millis, void *> timer;

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

bool resetCallback(void *) {
    ESP.restart();
    Serial.println('Rebooting...');
    return false;
}
static esp_err_t resetHandler(httpd_req_t *req) {
    const char resp[] = "Rebooting";
    httpd_resp_send(req, resp, strlen(resp));
    timer.in(2000, resetCallback);
    return ESP_OK;
}
void registerReset() {
    httpd_uri_t uri = {
    .uri       = "/reset",
    .method    = HTTP_GET,
    .handler   = resetHandler,
    .user_ctx  = NULL
  };
  
  httpd_register_uri_handler(server, &uri);
}

void initHTTP(int port = 80) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    httpd_start(&server, &config);

    registerReset();

    Serial.print("Ready at: http://");
    Serial.println(WiFi.localIP());
}