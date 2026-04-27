String writeFile(unsigned char * buf, unsigned int len, String path);
bool initSD();
bool stopSD();

#include "constants.h"
#include <WiFi.h>

#include <arduino-timer.h>
Timer<1, millis, void *> timer;

// HTTP server 
//
#include "esp_http_server.h"
httpd_handle_t server = NULL;

// NTP
//
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// SD Card
//
#include "SD.h"  

//disable brownout problems
//
#include "soc/soc.h" 
#include "soc/rtc_cntl_reg.h" 

// setup camera/motion library
//
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER
#define MOTION_DEBUG 0
#include "motion.h" 

#include "stream.h" 

void setup(void) {
    Serial.begin(115200);

    startWifi();
    timeClient.begin();
    setTime();

    setup_camera();

    startHTTP();
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

String writeFile(unsigned char * buf, unsigned int len, String path) { 
    String savedFile = "";

    if (initSD()) {
        fs::FS &fs = SD;
        Serial.printf("Picture file name: %s\n", path.c_str());

        File file = fs.open(path.c_str(), FILE_WRITE);
        if(!file) {
            Serial.println("Failed to open file in writing mode");
        } else {
            file.write(buf, len); 
            Serial.printf("Saved file to path: %s\n", path.c_str());
            savedFile = path;
        }
        file.close();

    } else Serial.println("No SD Card Attached");

    stopSD();
    return savedFile;
}

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

void startHTTP() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    httpd_start(&server, &config);

    registerStream();   
    registerCapture();
    registerReset();

    Serial.print("Ready at: http://");
    Serial.println(WiFi.localIP());
}

static esp_err_t reset_handler(httpd_req_t *req){
    const char resp[] = "Rebooting";
    httpd_resp_send(req, resp, strlen(resp));
    ESP.restart();
    return ESP_OK;
}
void registerReset() {
    httpd_uri_t uri = {
    .uri       = "/reset",
    .method    = HTTP_GET,
    .handler   = reset_handler,
    .user_ctx  = NULL
  };
  
  httpd_register_uri_handler(server, &uri);
}

bool initSD() {
    SPI.end();
    SD.end();

    SPI.begin(14, 2, 15, 13);
    if(!SD.begin(13)) {
        Serial.println("SD Card Mount Failed");
        return false;
    }

    if(SD.cardType() == CARD_NONE) {
        Serial.println("No SD Card attached");
        return false;
    }
    Serial.println("SD Card Detected.");
    return true;
}

bool stopSD() {
    SD.end();
    SPI.end();
}

void loop() {
    //if (is_capture) motion_loop();
    delay(1);
    timer.tick();
}