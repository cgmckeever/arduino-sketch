#include "esp_camera.h"
#include "camera_pins.h"

#include "fd_forward.h"

void getSettings();

bool cameraOK = false;
bool *cameraInUse = new bool(false);

enum cameraModes { isReady, isStream, isCapture, isMotion };
cameraModes *cameraMode = new cameraModes;

bool initCamera() {
    camera_config_t cameraConfig;

    cameraConfig.ledc_channel = LEDC_CHANNEL_0;
    cameraConfig.ledc_timer = LEDC_TIMER_0;
    cameraConfig.pin_d0 = Y2_GPIO_NUM;
    cameraConfig.pin_d1 = Y3_GPIO_NUM;
    cameraConfig.pin_d2 = Y4_GPIO_NUM;
    cameraConfig.pin_d3 = Y5_GPIO_NUM;
    cameraConfig.pin_d4 = Y6_GPIO_NUM;
    cameraConfig.pin_d5 = Y7_GPIO_NUM;
    cameraConfig.pin_d6 = Y8_GPIO_NUM;
    cameraConfig.pin_d7 = Y9_GPIO_NUM;
    cameraConfig.pin_xclk = XCLK_GPIO_NUM;
    cameraConfig.pin_pclk = PCLK_GPIO_NUM;
    cameraConfig.pin_vsync = VSYNC_GPIO_NUM;
    cameraConfig.pin_href = HREF_GPIO_NUM;
    cameraConfig.pin_sscb_sda = SIOD_GPIO_NUM;
    cameraConfig.pin_sscb_scl = SIOC_GPIO_NUM;
    cameraConfig.pin_pwdn = PWDN_GPIO_NUM;
    cameraConfig.pin_reset = RESET_GPIO_NUM;

    //cameraConfig.xclk_freq_hz = 20000000;
    //cameraConfig.xclk_freq_hz = 5000000;
    cameraConfig.xclk_freq_hz = config.camera_xclk_freq_hz;

    cameraConfig.jpeg_quality = 10;
    cameraConfig.fb_count = 1;

    cameraConfig.frame_size = FRAMESIZE_UXGA;
    cameraConfig.pixel_format = PIXFORMAT_JPEG;

    cameraOK = esp_camera_init(&cameraConfig) == ESP_OK;

    sensor_t *s = esp_camera_sensor_get();
    s->set_aec_value(s, config.camera_exposure);
    s->set_exposure_ctrl(s, config.camera_exposure_control);

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

/* https://randomnerdtutorials.com/esp32-cam-ov2640-camera-settings/ */
void updateParam(String param, int value) {
    sensor_t *s = esp_camera_sensor_get();

    if(param == "framesize") {
        s->set_framesize(s, (framesize_t) value);
    } else if (param ==  "hmirror") {
        s->set_hmirror(s, value);
        config.camera_hmirror = value;
    } else if (param == "vflip") {
        s->set_vflip(s, value);
        config.camera_vflip = value;
    } else if (param == "quality") {
        s->set_quality(s, value);
    } else if (param == "contrast") {
        s->set_contrast(s, value);
    } else if (param == "brightness") {
        s->set_brightness(s, value);
    } else if (param == "saturation") {
        s->set_saturation(s, value);
    } else if (param == "gainceiling") {
        s->set_gainceiling(s, (gainceiling_t) value);
    } else if (param == "colorbar") {
        s->set_colorbar(s, value);
    } else if (param == "awb") {
        s->set_whitebal(s, value);
    } else if (param == "agc") {
        s->set_gain_ctrl(s, value);
    } else if (param == "aec") {
        s->set_exposure_ctrl(s, value);
        config.camera_exposure_control = value;
        if (value == 0) s->set_aec_value(s, config.camera_exposure);
    } else if (param == "awb_gain") {
        s->set_awb_gain(s, value);
    } else if (param == "agc_gain") {
        s->set_agc_gain(s, value);
    } else if (param == "aec_value") {
        s->set_exposure_ctrl(s, 0);
        config.camera_exposure_control = 0;
        s->set_aec_value(s, value);
        config.camera_exposure = value;
    } else if (param == "aec2") {
       s->set_aec2(s, value);
    } else if (param == "dcw") {
        s->set_dcw(s, value);
    } else if (param == "bpc") {
        s->set_bpc(s, value);
    } else if (param == "wpc") {
        s->set_wpc(s, value);
    } else if (param == "lenc") {
        s->set_lenc(s, value);
    } else if (param == "raw_gma") {
        s->set_raw_gma(s, value);
    } else if (param == "special_effect") {
        s->set_special_effect(s, value);
    } else if (param == "wb_mode") {
        s->set_wb_mode(s, value);
    } else if (param == "ae_level") {
        s->set_ae_level(s, value);
    }
}

String configJSON() {
    StaticJsonDocument<1024> doc;
    JsonObject camera = doc.to<JsonObject>();

    sensor_t *s = esp_camera_sensor_get();
    camera["framesize"] = s->status.framesize;
    camera["quality"] = s->status.quality;
    camera["brightness"] = s->status.brightness;
    camera["contrast"] = s->status.contrast;
    camera["saturation"] = s->status.saturation;
    camera["sharpness"] = s->status.sharpness;
    camera["special_effect"] = s->status.special_effect;
    camera["wb_mode"] = s->status.wb_mode;
    camera["awb"] = s->status.awb;
    camera["awb_gain"] = s->status.awb_gain;
    camera["aec"] = s->status.aec;
    camera["aec2"] = s->status.aec2;
    camera["ae_level"] = s->status.ae_level;
    camera["aec_value"] = s->status.aec_value;
    camera["agc"] = s->status.agc;
    camera["agc_gain"] = s->status.agc_gain;
    camera["gainceiling"] = s->status.gainceiling;
    camera["bpc"] = s->status.bpc;
    camera["wpc"] = s->status.wpc;
    camera["raw_gma"] = s->status.raw_gma;
    camera["lenc"] = s->status.lenc;
    camera["vflip"] = s->status.vflip;
    camera["hmirror"] = s->status.hmirror;
    camera["dcw"] = s->status.dcw;
    camera["colorbar"] = s->status.colorbar;

    String json;
    serializeJson(camera, json);
    return json;
}
