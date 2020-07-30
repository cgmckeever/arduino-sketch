String saveFile(unsigned char*, unsigned int, String);
void captureSend(uint8_t*&, size_t&);
void registerCameraServer(int);

#include "standard.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera.h"

#define MOTION_DEBUG false
#define motionTriggerLevel 3
int resetTriggers = motionTriggerLevel * -2;
int alertsSent = 0;
#include "motion.h"

struct argsSend {
    uint8_t *buf;
    size_t len;
};
Timer<5, millis, argsSend*> sendTimer;
Timer<1, millis, void*> motionTimer;

void setup(void) {
    Serial.begin(115200);
    initSD();

    startWifi();
    timeClient.begin();
    setTime();

    initCamera();
    flash(false);

    initHTTP(80);
    registerCameraServer(81);

    motionTimer.every(500, timedMotion);
}

void loop() {
    sendTimer.tick();
    motionTimer.tick();
    timer.tick();
    //motionLoop();
}

bool timedMotion(void *) {
    if (motionLoop()) {
        if (motionTriggers >= motionTriggerLevel) {
            uint8_t* buf;
            size_t len;
            captureSend(buf, len);
            motionTriggers = 0;
        }
    }
    return true;
}

bool timedSend(argsSend *args) {
    Serial.println("Delay Send");
    saveSend(args->buf, args->len);
    delete args;
    return false;
}
void saveSend(uint8_t* buf, size_t len) {

    if (buf) {
        String path = "/picture." + String(time(NULL)) + "." + esp_random() + ".jpg";
        path = saveFile(buf, len, path); 
        if (path == "") Serial.println("Photo failed to save.");
        flash(false);

        SMTPData smtp;
        smtp.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);
        smtp.setSender("ESP32", emailSenderAccount);
        smtp.addRecipient(emailAlertAddress);
        smtp.setPriority("High");
        smtp.setSubject("Motion Detected " + path);
        smtp.setMessage("<div style=\"color:#2f4468;\"><h1>Hello World!</h1><p>- Sent from ESP32 board</p></div>", true);

        Serial.println("Attaching: " + path);
        smtp.addAttachData(path, "image/jpg", buf, len);

        if (MailClient.sendMail(smtp)) {
            Serial.println("Alert Sent");
        } else Serial.println("Error sending Email, " + MailClient.smtpErrorReason());

        smtp.empty();
        alertsSent += 1;
    } else Serial.println("No Capture Returned");
}

static esp_err_t indexHandler(httpd_req_t *req){
    Serial.println("/index");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");

    return httpd_resp_send(req, (const char *)index_html_gz, index_html_gz_len);
}

void captureSend(uint8_t*& buf, size_t& len) {
    disableMotion();

    sensor_t *sensor = esp_camera_sensor_get();
    sensor->set_pixformat(sensor, PIXFORMAT_JPEG);
    sensor->set_framesize(sensor, FRAMESIZE_UXGA);

    //flash(true);
    capture(buf, len);
    flash(false);
    
    argsSend *args = new argsSend();
    args->buf = buf;
    args->len = len;
    sendTimer.in(500, timedSend, args);
    enableMotion(resetTriggers);
}

static esp_err_t captureHandler(httpd_req_t *req){
    Serial.println("/capture");
    
    uint8_t* buf;
    size_t len;
    captureSend(buf, len);
    
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    return httpd_resp_send(req, (const char *)buf, len);
}

httpd_handle_t streamServer = NULL;
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
static esp_err_t streamHandler(httpd_req_t *req){
    Serial.println("/stream");
    disableMotion();
    
    uint8_t* buf;
    size_t len;
    char * part_buf[64];
    esp_err_t res = ESP_OK;

    httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    sensor_t *sensor = esp_camera_sensor_get();
    sensor->set_pixformat(sensor, PIXFORMAT_JPEG);
    //sensor->set_pixformat(sensor, PIXFORMAT_GRAYSCALE);
    sensor->set_framesize(sensor, FRAMESIZE_VGA);

    while(res == ESP_OK) { 
        capture(buf, len);
      
        if(buf) {
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
            if(res == ESP_OK) {
                res = httpd_resp_send_chunk(req, (const char *)buf, len);
                if(res == ESP_OK) {
                    res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
                }
            }
        }
    }

    Serial.println("end stream");
    enableMotion(resetTriggers);
    return res;
}
void registerCameraServer(int streamPort) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = streamPort;
    config.ctrl_port = streamPort * 1000 + streamPort;

    httpd_uri_t streamURI = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = streamHandler,
        .user_ctx  = NULL
    };

    if (httpd_start(&streamServer, &config) == ESP_OK) {
        httpd_register_uri_handler(streamServer, &streamURI);
    }
    Serial.printf("Starting stream server on port: %d\n", config.server_port);

    httpd_uri_t indexURI = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = indexHandler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &indexURI);

    httpd_uri_t captureURI = {
        .uri       = "/capture",
        .method    = HTTP_GET,
        .handler   = captureHandler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &captureURI);
}

