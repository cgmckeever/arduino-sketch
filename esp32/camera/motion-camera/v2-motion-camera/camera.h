#include "esp_camera.h"
#include "camera_pins.h"

#include "fd_forward.h"

void getSettings();

bool cameraOK = false;
bool *cameraInUse = new bool(false);

enum cameraModes { isReady, isStream, isCapture, isMotion };
cameraModes *cameraMode = new cameraModes;

bool initCamera() {
    camera_config_t config;

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    //config.xclk_freq_hz = 20000000;
    config.xclk_freq_hz = 10000000;
    //config.xclk_freq_hz = 5000000;

    config.jpeg_quality = 10;
    config.fb_count = 1;

    config.frame_size = FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG;

    cameraOK = esp_camera_init(&config) == ESP_OK;
    *cameraMode = isReady;
    *cameraInUse = false;
    return cameraOK;
}

void flash(bool on) {
    gpio_pad_select_gpio(GPIO_NUM_4);
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_4, on ? 1 : 0);
}

void cameraControl(cameraModes current) {
    *cameraMode = current;
    *cameraInUse = true;
}

void cameraRelease(cameraModes current) {
    if (*cameraMode == current) *cameraMode = isReady;
    *cameraInUse = false;
}

void bufferRelease(camera_fb_t* fb) {
    uint8_t *buf;
    buf = fb->buf;
    *buf = NULL;
    esp_camera_fb_return(fb);
}

camera_fb_t* capture(uint8_t*& _jpg_buf, size_t& _jpg_buf_len) {
    _jpg_buf_len = 0;

    camera_fb_t *fb = esp_camera_fb_get();

    if (fb) {
        if(fb->format == PIXFORMAT_JPEG) {
            _jpg_buf = fb->buf;
            _jpg_buf_len = fb->len;
        } else frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);

    } else loggerln("Camera capture failed");

    return fb;
}

String configJSON() {
    StaticJsonDocument<1024> doc;
    JsonArray config = doc.to<JsonArray>();
    JsonObject param = config.createNestedObject();

    sensor_t *s = esp_camera_sensor_get();
    param["framesize"] = s->status.framesize;
    param["quality"] = s->status.quality;
    param["brightness"] = s->status.brightness;
    param["contrast"] = s->status.contrast;
    param["saturation"] = s->status.saturation;
    param["sharpness"] = s->status.sharpness;
    param["special_effect"] = s->status.special_effect;
    param["wb_mode"] = s->status.wb_mode;
    param["awb"] = s->status.awb;
    param["awb_gain"] = s->status.awb_gain;
    param["aec"] = s->status.aec;
    param["aec2"] = s->status.aec2;
    param["ae_level"] = s->status.ae_level;
    param["aec_value"] = s->status.aec_value;
    param["agc"] = s->status.agc;
    param["agc_gain"] = s->status.agc_gain;
    param["gainceiling"] = s->status.gainceiling;
    param["bpc"] = s->status.bpc;
    param["wpc"] = s->status.wpc;
    param["raw_gma"] = s->status.raw_gma;
    param["lenc"] = s->status.lenc;
    param["vflip"] = s->status.vflip;
    param["hmirror"] = s->status.hmirror;
    param["dcw"] = s->status.dcw;
    param["colorbar"] = s->status.colorbar;

    String json;
    serializeJson(config, json);
    return json;
}
