#include "standard.h"

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
//#define T_Camera_V17_VERSION
#define T_Camera_JORNAL_VERSION

#include "camera.h"
#include "stream.h"

/** == used for capture/chunk streaming == **/
uint8_t* captureBuf;
size_t captureLen;
camera_fb_t *captureFB;


/* == motion.h == */
/*
#define MOTION_DEBUG false
#define motionTriggerLevel 2
time_t lastMotionAlertAt = 0;
int resetTriggers = 0;
int alertsSent = 0;
#include "motion.h"
*/

String saveFile(unsigned char*, unsigned int, String);
camera_fb_t* captureSend(uint8_t*&, size_t&);
void registerCameraServer(int);

int streamWait;

struct argsSend {
    String path;
};

//Timer<5, millis, argsSend*> sendTimer;
//Timer<1, millis, void*> motionTimer;
//Timer<1, millis, void*> socketTimer;

void setup(void) {
    Serial.begin(115200);
    DEBUG_MODE = true;
    //initSD();

    configSetup();
    //configDefaults();

    if (wifiConnected) {
        timeClient.begin();
        setTime();
        //if (config.sendAlerts) bootNotify();

        //configManager.stopWebserver();
        //registerCameraServer();
        //initHTTP(80);

        //motionTimer.every(500, timedMotion);
    }

    //streamWait = setStreamWait();
    //initCamera();
    //flash(false);

    configManager.stopWebserver();
    streamSetup();
}

void loop() {

    configManager.loop();

    /*
    sendTimer.tick();
    timer.tick();


    motionTimer.tick();
    //motionLoop();

    sockets();

    if (logSocket.count() > 0 || config.disableCameraMotion) {
        disableMotion();
    } else enableMotion();
    */

    vTaskDelay(1000);
}

/*
bool timedMotion(void*) {
    if (*cameraMode == isReady && !motionDisabled && !config.disableCameraMotion) {
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
*/

void send(String path="") {
    if (!wifiConnected) return;

    loggerln("Prepping Email");

    SMTPData smtp;
    smtp.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);
    smtp.setSender("ESP32", emailSenderAccount);
    smtp.addRecipient(emailAlertAddress);
    smtp.setPriority("High");
    smtp.setSubject((String) config.deviceName + " Motion Detected " + path);
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
    //alertsSent += 1;
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
    sensor->set_framesize(sensor, (framesize_t) config.captureFramesize);

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

    if (config.sendAlerts) {
        argsSend *args = new argsSend();
        args->path = path;
        //sendTimer.in(500, captureCallback, args);
    }

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
            loggerln("Start image spool");
            Serial.printf(PSTR("[WEB] Sending chunked buffer (max chunk size: %4d) "), max);
            Serial.println("");
        }
        memcpy_P(buffer, captureBuf + index, len);
    } else {
        *captureBuf = NULL;
        bufferRelease(captureFB);
        cameraRelease(isCapture);
    }

    return len;
}

/*
void registerCameraServer() {
    webServer.addHandler(&streamSocket);

    webServer.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        loggerln("GET /config");

        StaticJsonDocument<1024> doc;
        JsonObject json = doc.to<JsonObject>();

        DynamicJsonDocument camera(1024);
        auto error = deserializeJson(camera, configJSON());
        json["camera"] = camera;
        json["config"] = configManager.asJson();

        String jsonResponse;
        serializeJson(json, jsonResponse);

        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonResponse);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });

    // https://github.com/me-no-dev/ESPAsyncWebServer/issues/195
    webServer.on("/config", HTTP_PUT, [](AsyncWebServerRequest *request) {},
        NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            loggerln("PUT /config");
            DynamicJsonDocument doc(1024);
            auto error = deserializeJson(doc, (const char*)data);

            if (error) {
                request->send(400);
                return;
            }

            JsonObject json = doc.as<JsonObject>();

            if (json.containsKey("camera")) {
                JsonObject camera = json["camera"];
                for (JsonPair kv : camera) {
                    updateParam(kv.key().c_str(), kv.value().as<int>());
                }
            }

            if (json.containsKey("config")) {
                JsonObject configs = json["config"];
                for (JsonPair kv : configs) {
                    String key = kv.key().c_str();
                    loggerln("Device Param: " + key);
                    if (key == "captureFramesize") {
                        config.captureFramesize = kv.value().as<int>();
                    } else if (key == "streamFramesize") {
                        config.streamFramesize = kv.value().as<int>();
                    }
                }
            }

            configManager.save();

            request->send(200);
    });

    webServer.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
        loggerln("/capture");
        *cameraMode = isCapture;
        streamWait = 1000;

        int waitStart = millis();
        while (*cameraInUse == true) {
            *cameraMode = isCapture;
            streamWait = 1000;
            if (millis() - waitStart > 1000) {
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
        sensor->set_framesize(sensor, (framesize_t) config.captureFramesize);

        captureFB = capture(captureBuf, captureLen);
        String path = saveBuffer(captureBuf, captureLen, "jpg");

        AsyncWebServerResponse *response = request->beginChunkedResponse("image/jpeg", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
            return chunkBuffer((char *) buffer, maxLen, index);
        });
        response->addHeader("Content-Disposition", "inline; filename=" + path);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });
}

*/


