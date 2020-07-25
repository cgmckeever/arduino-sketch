//
// https://github.com/eloquentarduino/EloquentArduino/tree/master/examples/ESP32CameraNaiveMotionDetection
//
//////////

#include "esp_camera.h"
#include "camera_pins.h"

#define WIDTH 320
#define HEIGHT 240
#define BLOCK_SIZE 10
#define W (WIDTH / BLOCK_SIZE)
#define H (HEIGHT / BLOCK_SIZE)
#define BLOCK_DIFF_THRESHOLD 0.2
#define IMAGE_DIFF_THRESHOLD 0.1

#if !defined(MOTION_DEBUG)
#define MOTION_DEBUG 0
#endif


uint16_t prev_frame[H][W] = { 0 };
uint16_t current_frame[H][W] = { 0 };
bool is_capture = true;

bool setup_camera();

bool capture_still();
bool motion_detect();
void update_frame();

void print_frame(uint16_t frame[H][W]);
void print_state(uint16_t changes, uint16_t blocks);
void print_settings(sensor_t * sensor);


/**
 *
 */
void motion_loop() {
    if (!is_capture) return;

    if (!capture_still()) {
        Serial.println("Failed capture");
        delay(3000);
        return;
    }

    if (motion_detect()) Serial.println("Motion detected");

    update_frame();
}


/**
 *
 */
bool setup_camera() {
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

    //config.frame_size = FRAMESIZE_UXGA;
    //config.pixel_format = PIXFORMAT_JPEG;

    return esp_camera_init(&config) == ESP_OK;
}

void print_settings(sensor_t * sensor) {
    Serial.println("=====================");
    Serial.println(sensor->status.framesize);
    Serial.println(sensor->status.quality);
    Serial.println(sensor->status.brightness);
    Serial.println(sensor->status.contrast);
    Serial.println(sensor->status.saturation);
    Serial.println(sensor->status.sharpness);
    Serial.println(sensor->status.special_effect);
    Serial.println(sensor->status.wb_mode);
    Serial.println(sensor->status.awb);
    Serial.println(sensor->status.awb_gain);
    Serial.println(sensor->status.aec);
    Serial.println(sensor->status.aec2);
    Serial.println(sensor->status.ae_level);
    Serial.println(sensor->status.aec_value);
    Serial.println(sensor->status.agc);
    Serial.println(sensor->status.agc_gain);
    Serial.println(sensor->status.gainceiling);
    Serial.println(sensor->status.bpc);
    Serial.println(sensor->status.wpc);
    Serial.println(sensor->status.raw_gma);
    Serial.println(sensor->status.lenc);
    Serial.println(sensor->status.vflip);
    Serial.println(sensor->status.hmirror);
    Serial.println(sensor->status.dcw);
    Serial.println(sensor->status.colorbar);
}
  

/**
 * Capture image and do down-sampling
 */
bool capture_still() {
    sensor_t * sensor = esp_camera_sensor_get();
    sensor->set_pixformat(sensor, PIXFORMAT_GRAYSCALE);
    sensor->set_framesize(sensor, FRAMESIZE_QVGA);

    camera_fb_t *frame_buffer = esp_camera_fb_get();
    esp_camera_fb_return(frame_buffer);

    if (!frame_buffer) return false;

    // set all 0s in current frame
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            current_frame[y][x] = 0;


    // down-sample image in blocks
    for (uint32_t i = 0; i < WIDTH * HEIGHT; i++) {
        const uint16_t x = i % WIDTH;
        const uint16_t y = floor(i / WIDTH);
        const uint8_t block_x = floor(x / BLOCK_SIZE);
        const uint8_t block_y = floor(y / BLOCK_SIZE);
        const uint8_t pixel = frame_buffer->buf[i];
        const uint16_t current = current_frame[block_y][block_x];

        // average pixels in block (accumulate)
        current_frame[block_y][block_x] += pixel;
    }

    // average pixels in block (rescale)
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            current_frame[y][x] /= BLOCK_SIZE * BLOCK_SIZE;

#if MOTION_DEBUG
    Serial.println("Current frame:");
    print_frame(current_frame);
    Serial.println("---------------");
#endif

    return true;
}


/**
 * Compute the number of different blocks
 * If there are enough, then motion happened
 */
bool motion_detect() {
    uint16_t changes = 0;
    const uint16_t blocks = (WIDTH * HEIGHT) / (BLOCK_SIZE * BLOCK_SIZE);

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float current = current_frame[y][x];
            float prev = prev_frame[y][x];
            float delta = abs(current - prev) / prev;

            if (delta >= BLOCK_DIFF_THRESHOLD) {
                changes += 1;

                if (MOTION_DEBUG) {
                    Serial.print("diff\t");
                    Serial.print(y);
                    Serial.print('\t');
                    Serial.println(x);
                }
            }
        }
    }

    bool motion_detected = (1.0 * changes / blocks) > IMAGE_DIFF_THRESHOLD;

    if (MOTION_DEBUG || motion_detected) {
        print_state(changes, blocks);
    }

    return motion_detected;
}


/**
 * Copy current frame to previous
 */
void update_frame() {
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            prev_frame[y][x] = current_frame[y][x];
        }
    }
}

void print_state(uint16_t changes, uint16_t blocks) {
    Serial.print("Changed ");
    Serial.print(changes);
    Serial.print(" out of ");
    Serial.println(blocks);
    Serial.println("=================");
}

/**
 * For serial debugging
 * @param frame
 */
void print_frame(uint16_t frame[H][W]) {
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            Serial.print(frame[y][x]);
            Serial.print('\t');
        }

        Serial.println();
    }
}