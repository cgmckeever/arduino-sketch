#include "esp_camera.h"
#include "camera_pins.h"
#include "html.h"

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
    config.xclk_freq_hz = 20000000;

    config.jpeg_quality = 10;
    config.fb_count = 1;

    config.frame_size = FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG;

    cameraOK = esp_camera_init(&config) == ESP_OK;
    return cameraOK;
} 

camera_fb_t* capture() {
    sensor_t *sensor = esp_camera_sensor_get();
    sensor->set_pixformat(sensor, PIXFORMAT_JPEG);
    sensor->set_framesize(sensor, FRAMESIZE_UXGA);

    gpio_pad_select_gpio(GPIO_NUM_4);
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);

    gpio_set_level(GPIO_NUM_4, 1);
    camera_fb_t *fb = esp_camera_fb_get(); 
    gpio_set_level(GPIO_NUM_4, 0);

    if (!fb) Serial.println("Camera capture failed");

    return fb;
}


static esp_err_t indexHandler(httpd_req_t *req){
    Serial.println('/index');
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");

    return httpd_resp_send(req, (const char *)index_html_gz, index_html_gz_len);
}
void registerCameraServer(int port = 80){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t indexURI = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = indexHandler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &indexURI);
}
