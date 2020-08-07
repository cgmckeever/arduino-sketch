#include "standard.h"
String deviceName = (String) config.deviceName;

AsyncWebServer webServer(80);
AsyncWebSocket webSocket("/ws");
int lastStreamTime = 0;

/*
#include <ArduinoWebsockets.h>
using namespace websockets;
WebsocketsServer sockets;
*/

#define CAMERA_MODEL_AI_THINKER
#include "camera.h"

camera_fb_t *fb = NULL;

/* == motion.h ==*/
#define MOTION_DEBUG false
#define motionTriggerLevel 2
time_t lastMotionAlertAt = 0;
int resetTriggers = 0;
int alertsSent = 0;
#include "motion.h"

String saveFile(unsigned char*, unsigned int, String);
pixformat_t captureSend(uint8_t*&, size_t&);
void registerCameraServer(int);

struct argsSend {
    String path;
};

Timer<5, millis, argsSend*> sendTimer;
Timer<1, millis, void*> motionTimer;
Timer<1, millis, void*> socketTimer;

TaskHandle_t Task1;


void setup(void) {
    Serial.begin(115200);
    DEBUG_MODE = true;
    initSD();

    configSetup();

    if (wifiConnected) {
        timeClient.begin();
        setTime();

        //bootNotify();

        configManager.stopWebserver();

        //webSocket.onEvent(onWsEvent);
        max_ws_queued_messages = 3;
        webServer.addHandler(&webSocket);
        initHTTP(80);

        //sockets.listen(82);
        //socketTimer.every(200, timedSocket);

        //registerCameraServer(81);
    }

    initCamera();
    flash(false);

    //motionTimer.every(500, timedMotion);
    sensor_t *sensor = esp_camera_sensor_get();
    sensor->set_pixformat(sensor, PIXFORMAT_JPEG);
    sensor->set_framesize(sensor, FRAMESIZE_CIF);

/*
    xTaskCreatePinnedToCore(
      espSocket, // Function to implement the task
      "Task1", // Name of the task
      10000,  // Stack size in words
      NULL,  // Task input parameter
      0,  //Priority of the task
      &Task1,  // Task handle.
      0); // Core where the task should run
      */
}

/*
bool arduinoSocket() {
    auto client = sockets.accept();
    //client.onMessage(handle_message);
    while (client.available()) {
        client.poll();
        fb = esp_camera_fb_get();
        client.sendBinary((const char *)fb->buf, fb->len);
        esp_camera_fb_return(fb);
        fb = NULL;
    }
    return true;
}
*/


void espSocket() {

    if ((millis() - lastStreamTime) > 100) {
        /*
        AsyncWebSocket::AsyncWebSocketClientLinkedList clients = webSocket.getClients();
        size_t queue = 0;
        int connected = 0;
        for(const auto& c: clients) {
            queue += c->queueLength();
            connected += 1;
        }

        if (connected > 0 && queue < 3) {
            lastStreamTime = millis();
            uint8_t* buf;
            size_t len;
            capture(buf, len);
            webSocket.binaryAll(buf, len);
        }
        */
        lastStreamTime = millis();
        uint8_t* buf;
        size_t len;
        capture(buf, len);
        webSocket.binaryAll(buf, len);
    }

    //fb = esp_camera_fb_get();
    //webSocket.binaryAll(fb->buf, fb->len);

    //webSocket.binaryAll((const char *) fb->buf);
    //webSocket.binary(client->id(), fb->buf, fb->len);
    //esp_camera_fb_return(fb);
}

void loop() {
    sendTimer.tick();
    //motionTimer.tick();
    timer.tick();
    configManager.loop();
    //motionLoop();

    //arduinoSocket();
    espSocket();

    webSocket.cleanupClients();

}

bool timedMotion(void*) {
    if (motionLoop()) {
        if (motionTriggers >= motionTriggerLevel) {
            if (time(NULL) - lastMotionAlertAt > 30) {
                uint8_t* buf;
                size_t len;
                pixformat_t pixformat = captureSend(buf, len);
                if (pixformat != PIXFORMAT_JPEG) free(buf);
                lastMotionAlertAt = time(NULL);
            } else Serial.println("Motion Alert Skipped");

            motionTriggers = 0;
        }
    }
    return true;
}

void send(String path="") {
    if (!wifiConnected) return;

    SMTPData smtp;
    smtp.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);
    smtp.setSender("ESP32", emailSenderAccount);
    smtp.addRecipient(emailAlertAddress);
    smtp.setPriority("High");
    smtp.setSubject((String) deviceName + " Motion Detected " + path);
    smtp.setMessage("<div style=\"color:#2f4468;\"><h1>Hello World!</h1><p>- Sent from ESP32 board</p></div>", true);

    if (path != "") {
        Serial.println("Attaching: " + path);
        smtp.addAttachFile(path);
        smtp.setFileStorageType(MailClientStorageType::SD);
    }

    if (MailClient.sendMail(smtp)) {
        Serial.println("Alert Sent");
    } else Serial.println("Error sending Email, " + MailClient.smtpErrorReason());

    smtp.empty();
    alertsSent += 1;
}


/*
static esp_err_t indexHandler(httpd_req_t *req){
    Serial.println("/index");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");

    return httpd_resp_send(req, (const char *)index_html_gz, index_html_gz_len);
} */

bool captureCallback(argsSend *args) {
    Serial.println("Delay Send");
    send(args->path);
    enableMotion(resetTriggers);
    delete args;
    return false;
}
pixformat_t captureSend(uint8_t*& buf, size_t& len) {
    disableMotion();

    pixformat_t format;
    pixformat_t pixformat = PIXFORMAT_JPEG;
    //pixformat_t pixformat = PIXFORMAT_GRAYSCALE;

    sensor_t *sensor = esp_camera_sensor_get();
    sensor->set_pixformat(sensor, pixformat);
    sensor->set_framesize(sensor, FRAMESIZE_UXGA);

    //flash(true);
    int counter = 0;
    format = capture(buf, len);
    while (counter < 6 && format != pixformat) {
        counter += 1;
        if (format != PIXFORMAT_JPEG) free(buf);
        format = capture(buf, len);
    }
    flash(false);

    String path = "/picture." + String(time(NULL)) + "." + esp_random() + ".jpg";
    path = saveFile(buf, len, path);
    if (path == "") Serial.println("Photo failed to save.");
    flash(false);

    argsSend *args = new argsSend();
    args->path = path;
    sendTimer.in(500, captureCallback, args);

    return pixformat;
}

/*
static esp_err_t captureHandler(httpd_req_t *req){
    Serial.println("/capture");

    uint8_t* buf;
    size_t len;
    pixformat_t pixformat = captureSend(buf, len);

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    esp_err_t res = httpd_resp_send(req, (const char *)buf, len);

    if (pixformat != PIXFORMAT_JPEG) free(buf);

    return res;
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
    pixformat_t pixformat = PIXFORMAT_JPEG;
    //pixformat_t pixformat = PIXFORMAT_GRAYSCALE;

    httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    sensor_t *sensor = esp_camera_sensor_get();
    sensor->set_pixformat(sensor, pixformat);
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
        if (pixformat != PIXFORMAT_JPEG) free(buf);
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
*/

