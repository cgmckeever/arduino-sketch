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
        if (*cameraMode == isReady && (millis() - lastStreamTime) > streamWait) {
            cameraControl(isStream);
            streamWait = 100;

            pixformat_t pixformat = PIXFORMAT_JPEG;
            //pixformat_t pixformat = PIXFORMAT_GRAYSCALE;

            sensor_t *sensor = esp_camera_sensor_get();
            sensor->set_pixformat(sensor, pixformat);
            sensor->set_framesize(sensor, FRAMESIZE_VGA);

            uint8_t *jpgBuf;
            size_t jpgLen;
            camera_fb_t *fb = capture(jpgBuf, jpgLen);

            if (fb) {
                max_ws_queued_messages = 3;

                streamSocket.binaryAll(jpgBuf, jpgLen);
                bufferRelease(fb);

                lastStreamTime = millis();
                if (fb->format != PIXFORMAT_JPEG) free(jpgBuf);
            }
            cameraRelease(isStream);
        }
    } else if (lastStreamTime > 0) {
        lastStreamTime = 0;
        enableMotion();
    }
}

void loop() {
    sendTimer.tick();
    timer.tick();
    configManager.loop();

    motionTimer.tick();
    //motionLoop();

    sockets();

    if (logSocket.count() > 0) {
        disableMotion();
    } else enableMotion();
}

bool timedMotion(void*) {
    if (*cameraMode == isReady && !motionDisabled) {
        cameraControl(isMotion);
        if (motionLoop()) {
            if (motionTriggers >= motionTriggerLevel) {
                if (time(NULL) - lastMotionAlertAt > 30) {
                    uint8_t *jpgBuf;
                    size_t jpgLen;
                    camera_fb_t *fb = captureSend(jpgBuf, jpgLen);
                    if (fb->format != PIXFORMAT_JPEG) free(jpgBuf);
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

bool captureCallback(argsSend *args) {
    loggerln("Delay Send");
    send(args->path);
    delete args;
    return false;
}
camera_fb_t* captureSend(uint8_t*& jpgBuf, size_t& jpgLen) {
    pixformat_t format;
    pixformat_t pixformat = PIXFORMAT_JPEG;
    //pixformat_t pixformat = PIXFORMAT_GRAYSCALE;

    sensor_t *sensor = esp_camera_sensor_get();
    sensor->set_pixformat(sensor, pixformat);
    sensor->set_framesize(sensor, FRAMESIZE_UXGA);

    //flash(true);
    int counter = 0;
    camera_fb_t *fb = capture(jpgBuf, jpgLen);

    /*
    while (counter < 6 && format != pixformat) {
        counter += 1;
        if (fb->format != PIXFORMAT_JPEG) free(buf);
        bufferRelease(fb);
        fb = capture(buf, len);
    }
    */
    flash(false);

    String path = saveBuffer(jpgBuf, jpgLen, "jpg");

    argsSend *args = new argsSend();
    args->path = path;
    sendTimer.in(500, captureCallback, args);

    return fb;
}

String saveBuffer(uint8_t* buf, size_t len, String ext) {
    String path = "/picture." + String(time(NULL)) + "." + esp_random() + "." + ext;
    path = saveFile(buf, len, path);
    flash(false);
    if (path == "") loggerln("Photo failed to save.");
    return path;
}


int chunkBuffer(char *buffer, size_t maxLen, size_t index)
{
    size_t max  = (ESP.getFreeHeap() / 3) & 0xFFE0;

    size_t len = captureLen - index;
    if (len > maxLen) len = maxLen;
    if (len > max) len = max;

    if (len > 0) {
        if (index == 0) {
            loggerln("Image Spool Complete.");
            Serial.printf(PSTR("[WEB] Sending chunked buffer (max chunk size: %4d) "), max);
        }
        memcpy_P(buffer, captureBuf + index, len);
    } else {
        *captureBuf = NULL;
        bufferRelease(captureFB);
        cameraRelease(isCapture);
    }

    return len;
}

void registerCameraServer() {
    webServer.addHandler(&streamSocket);

    webServer.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        loggerln("/config");
        request->send(200, "application/json", configJSON());
    });

    AsyncCallbackJsonWebHandler* configHandler = new AsyncCallbackJsonWebHandler("/rest/endpoint", [](AsyncWebServerRequest *request, JsonVariant &json) {
        //JsonObject& jsonObj = json.as<JsonObject>();
        // ...
    });
    webServer.addHandler(configHandler);

    webServer.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
        loggerln("/capture");
        *cameraMode = isCapture;
        streamWait = 1000;

        int waitStart = millis();
        while (*cameraInUse == true) {
            *cameraMode = isCapture;
            streamWait = 1000;
            if (millis() - waitStart > 3000) {
                request->send(200, "text/plain", "Capture Timeout");
                loggerln("Capture timeout");
                cameraRelease(isCapture);
                return;
            }
        }

        disableMotion();
        cameraControl(isCapture);

        // only capture JPEG, no free(buf)
        pixformat_t pixformat = PIXFORMAT_JPEG;
        sensor_t *sensor = esp_camera_sensor_get();
        sensor->set_pixformat(sensor, pixformat);
        sensor->set_framesize(sensor, FRAMESIZE_SVGA);

        captureFB = capture(captureBuf, captureLen);
        saveBuffer(captureBuf, captureLen, "jpg");

        AsyncWebServerResponse *response = request->beginChunkedResponse("image/jpeg", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
            return chunkBuffer((char *) buffer, maxLen, index);
        });
        response->addHeader("Content-Disposition", "inline; filename=capture.jpg");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });
}


