#include "constants.h"

#include <WiFi.h>

#include "esp_http_server.h"
httpd_handle_t server = NULL;

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
    startHTTP();
    
    bool camera = setup_camera();
    registerStream();   
    registerReset();
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

    //disable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1); 
}

void startHTTP() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    httpd_start(&server, &config);

    Serial.print("Ready at: http://");
    Serial.println(WiFi.localIP());
}

static esp_err_t reset_handler(httpd_req_t *req){
    ESP.restart();
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

void loop() {
    if (is_capture) motion_loop();
    delay(1);
}