#include "standard.h"
String deviceName = (String) config.deviceName;

/* == web/sockets ==*/
AsyncWebServer webServer(80);
AsyncWebSocket streamSocket("/stream");
int lastStreamTime = 0;

#define CAMERA_MODEL_AI_THINKER
#include "camera.h"

/** == used for capture/chunk streaming == **/
uint8_t* captureBuf;
size_t captureLen;
camera_fb_t *captureFB;

int streamWait = 100;

/* == motion.h ==*/
#define MOTION_DEBUG false
#define motionTriggerLevel 2
time_t lastMotionAlertAt = 0;
int resetTriggers = 0;
int alertsSent = 0;
#include "motion.h"

String saveFile(unsigned char*, unsigned int, String);
camera_fb_t* captureSend(uint8_t*&, size_t&);
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
    initSD();

    configSetup();

    if (wifiConnected) {
        timeClient.begin();
        setTime();

        //bootNotify();

        configManager.stopWebserver();

        registerCameraServer();
        initHTTP(80);
    }

    initCamera();
    flash(false);

    motionTimer.every(500, timedMotion);
}

void sockets() {
    if (streamSocket.count() > 0) {
        disableMotion();
        if (cameraMode == isReady && (millis() - lastStreamTime) > streamWait) {
            streamWait = 100;
            cameraMode = isStream;
            *cameraInUse = true;
            uint8_t* buf;
            size_t len;
            pixformat_t pixformat = PIXFORMAT_JPEG;
            //pixformat_t pixformat = PIXFORMAT_GRAYSCALE;

            sensor_t *sensor = esp_camera_sensor_get();
            sensor->set_pixformat(sensor, pixformat);
            sensor->set_framesize(sensor, FRAMESIZE_QVGA);

            camera_fb_t *fb = capture(buf, len);
            if (fb) {
                max_ws_queued_messages = 2;
                streamSocket.binaryAll(buf, len);
                bufferRelease(fb);
                cameraRelease(isStream);

                lastStreamTime = millis();
                if (fb->format != PIXFORMAT_JPEG) free(buf);
            }
            cameraInUse = false;
        }
    } else enableMotion();
}

void loop() {
    sendTimer.tick();
    timer.tick();
    configManager.loop();

    motionTimer.tick();
    //motionLoop();

    sockets();
}

bool timedMotion(void*) {
    if (cameraMode == isReady) {
        cameraMode = isMotion;
        if (motionLoop()) {
            if (motionTriggers >= motionTriggerLevel) {
                if (time(NULL) - lastMotionAlertAt > 30) {
                    uint8_t* buf;
                    size_t len;
                    camera_fb_t *fb = captureSend(buf, len);
                    if (fb->format != PIXFORMAT_JPEG) free(buf);
                    bufferRelease(fb);
                    lastMotionAlertAt = time(NULL);
                } else loggerln("Motion Alert Skipped");

                motionTriggers = 0;
            }
        }
        cameraRelease(isMotion);
    }

    return true;
}

void send(String path="") {
    if (!wifiConnected) return;

    loggerln("Prepping Email");

    SMTPData smtp;
    smtp.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);
    smtp.setSender("ESP32", emailSenderAccount);
    smtp.addRecipient(emailAlertAddress);
    smtp.setPriority("High");
    smtp.setSubject((String) deviceName + " Motion Detected " + path);
    smtp.setMessage("<div style=\"color:#2f4468;\"><h1>Hello World!</h1><p>- Sent from ESP32 board</p></div>", true);

    if (path != "") {
        loggerln("Attaching: " + path);
        smtp.addAttachFile(path);
        smtp.setFileStorageType(MailClientStorageType::SD);
    }

    if (MailClient.sendMail(smtp)) {
        loggerln("Alert Sent");
    } else loggerln("Error sending Email, " + MailClient.smtpErrorReason());

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
    loggerln("Delay Send");
    send(args->path);
    enableMotion(resetTriggers);
    delete args;
    return false;
}
camera_fb_t* captureSend(uint8_t*& buf, size_t& len) {
    disableMotion();

    pixformat_t format;
    pixformat_t pixformat = PIXFORMAT_JPEG;
    //pixformat_t pixformat = PIXFORMAT_GRAYSCALE;

    sensor_t *sensor = esp_camera_sensor_get();
    sensor->set_pixformat(sensor, pixformat);
    sensor->set_framesize(sensor, FRAMESIZE_UXGA);

    //flash(true);
    int counter = 0;
    camera_fb_t *fb = capture(buf, len);
    /*
    while (counter < 6 && format != pixformat) {
        counter += 1;
        if (fb->format != PIXFORMAT_JPEG) free(buf);
        bufferRelease(fb);
        fb = capture(buf, len);
    }
    */
    flash(false);

    String path = "/picture." + String(time(NULL)) + "." + esp_random() + ".jpg";
    path = saveFile(buf, len, path);
    if (path == "") loggerln("Photo failed to save.");
    flash(false);

    argsSend *args = new argsSend();
    args->path = path;
    sendTimer.in(500, captureCallback, args);

    return fb;
}


int chunkBuffer(char *buffer, size_t maxLen, size_t index)
{
    size_t max   = (ESP.getFreeHeap() / 3) & 0xFFE0;

    size_t len = captureLen - index;
    if (len > maxLen) len = maxLen;
    if (len > max) len = max;

    if (len > 0) {
        if (index == 0) {
            loggerln("Sending Image Complete.");
            Serial.printf(PSTR("[WEB] Sending chunked buffer (max chunk size: %4d) "), max);
        }
        memcpy_P(buffer, captureBuf + index, len);
    } else {
        bufferRelease(captureFB);
        cameraRelease(isCapture);
    }

    return len;
}

void registerCameraServer() {
    webServer.addHandler(&streamSocket);

    webServer.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
        loggerln("/capture");
        cameraMode = isCapture;
        streamWait = 1000;

        int waitStart = millis();
        while (*cameraInUse) {
            loggerln("Waiting for camera ready...");
            cameraMode = isCapture;
            streamWait = 1000;
            if (millis() - waitStart > 1200) {
                request->send(200, "text/plain", "Capture Timeout");
                loggerln("Capture timeout");
                cameraRelease(isCapture);
                return;
            }
        }

        // only capture JPEG, no free(buf)
        pixformat_t pixformat = PIXFORMAT_JPEG;
        sensor_t *sensor = esp_camera_sensor_get();
        sensor->set_pixformat(sensor, pixformat);
        sensor->set_framesize(sensor, FRAMESIZE_SVGA);

        captureFB = captureSend(captureBuf, captureLen);

        AsyncWebServerResponse *response = request->beginChunkedResponse("image/jpeg", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
            return chunkBuffer((char *) buffer, maxLen, index);
        });
        response->addHeader("Content-Disposition", "inline; filename=capture.jpg");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);

    });
}


