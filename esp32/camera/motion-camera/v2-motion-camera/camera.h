#include "esp_camera.h"
#include "camera_pins.h"
#include "html.h"

#include "fd_forward.h"

void getSettings();

bool cameraOK = false;

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
    return cameraOK;
} 

void flash(bool on) {
    gpio_pad_select_gpio(GPIO_NUM_4);
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_4, on ? 1 : 0);
}

pixformat_t capture(uint8_t*& _jpg_buf , size_t& _jpg_buf_len) {
    _jpg_buf_len = 0;
    pixformat_t format; 

    camera_fb_t *fb = esp_camera_fb_get();
    
    if (fb) {
        if(fb->format != PIXFORMAT_JPEG) {
            frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        } else {
            _jpg_buf = fb->buf;
            _jpg_buf_len = fb->len;
        }
        format = fb->format;
        esp_camera_fb_return(fb);
        fb = NULL;
    } else Serial.println("Camera capture failed");

    return format;
}

void get_chunk(uint8_t*& _jpg_buf , size_t& _jpg_buf_len){
    bool captured = false;
    _jpg_buf_len = 0;
    _jpg_buf = NULL;

    dl_matrix3du_t *image_matrix = NULL;

    sensor_t *sensor = esp_camera_sensor_get();
    sensor->set_pixformat(sensor, PIXFORMAT_JPEG);
    sensor->set_framesize(sensor, sensor->status.framesize);
    
    camera_fb_t *fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);

    if (fb) {
        if(fb->width > 400) {
            if(fb->format != PIXFORMAT_JPEG) {
                if(!frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len)) {
                    Serial.println("JPEG compression failed");
                }
            } else captured = true;
        } else {
            image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
            if (image_matrix) {
                if(fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item)) {
                    if (fb->format != PIXFORMAT_JPEG) {
                        if(!fmt2jpg(image_matrix->item, fb->width*fb->height*3, fb->width, fb->height, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len)) {
                            Serial.println("fmt2jpg failed");
                        }
                    } else captured = true;
                }
                dl_matrix3du_free(image_matrix);
            }
        }

        if (captured) {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }
    } else Serial.println("Camera capture failed");
}

void getSettings() {
    sensor_t * sensor = esp_camera_sensor_get();

    Serial.print("Framesize: ");
    Serial.println(sensor->status.framesize);
    Serial.print("Quality: ");
    Serial.println(sensor->status.quality);
    Serial.print("Effect: ");
    Serial.println(sensor->status.special_effect);
    Serial.println("=================================");
}
