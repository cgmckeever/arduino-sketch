//
// https://github.com/eloquentarduino/EloquentArduino/tree/master/examples/ESP32CameraNaiveMotionDetection
// https://gist.github.com/eloquentarduino/6bb0b26a3900d7fac68b2f3cc7b2c688
//
//////////

//
// based on framesize
//
////
#define WIDTH 320
#define HEIGHT 240
#define BLOCK_SIZE 20

#define W (WIDTH / BLOCK_SIZE)
#define H (HEIGHT / BLOCK_SIZE)
#define BLOCK_DIFF_THRESHOLD 0.05
#define IMAGE_DIFF_THRESHOLD 0.1

#if !defined(MOTION_DEBUG)
#define MOTION_DEBUG 0
#endif

uint16_t prev_frame[H][W] = { 0 };
uint16_t current_frame[H][W] = { 0 };
uint16_t averagePixelTemp = 0; // not implemented
bool disableMotion = false;

bool capture_still();
bool motion_detect();
void update_frame();

void print_frame(uint16_t frame[H][W]);
void print_state(uint16_t changes, uint16_t blocks);


bool motionDetect() {
    bool motionDetected = false; 

    if (!disableMotion) {
        if (!capture_still()) {
            Serial.println("Failed motion capture");
        } else {
            motionDetected = motion_detect();
            if (motionDetected) Serial.println("Motion detected");
            update_frame();
        }
    }

    return motionDetected;
}

void motionSettings() {
    sensor_t * sensor = esp_camera_sensor_get();
    if (sensor->status.framesize != FRAMESIZE_QVGA && sensor->pixformat != PIXFORMAT_GRAYSCALE) {
        sensor_t * sensor = esp_camera_sensor_get();
        sensor->set_pixformat(sensor, PIXFORMAT_GRAYSCALE);
        sensor->set_framesize(sensor, FRAMESIZE_QVGA);
        Serial.println("Motion Settings Enabled");
    }
}

bool capture_still() {
    motionSettings();

    camera_fb_t *frame_buffer = esp_camera_fb_get();
    esp_camera_fb_return(frame_buffer);
    if (!frame_buffer) return false;

    current_frame[H][W] = { 0 };
    uint32_t totalPixelTemp = 0;

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
        totalPixelTemp += pixel;
    }

    averagePixelTemp = totalPixelTemp / (WIDTH * HEIGHT); 

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