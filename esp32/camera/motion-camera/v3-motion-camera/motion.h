//
// https://github.com/eloquentarduino/EloquentArduino/tree/master/examples/ESP32CameraNaiveMotionDetection
// https://gist.github.com/eloquentarduino/6bb0b26a3900d7fac68b2f3cc7b2c688
//
//////////

//
// based on framesize
//
////
#define WIDTH 640
#define HEIGHT 480
#define BLOCK_SIZE 10
#define MOTION_FRAME FRAMESIZE_VGA

#define W (WIDTH / BLOCK_SIZE)
#define H (HEIGHT / BLOCK_SIZE)
#define BLOCK_DIFF_THRESHOLD 0.20
#define IMAGE_DIFF_THRESHOLD 0.10

#if !defined(MOTION_DEBUG)
#define MOTION_DEBUG false
#endif

uint16_t prev_frame[H][W] = { 0 };
uint16_t current_frame[H][W] = { 0 };
uint16_t averagePixelTemp = 0; // not implemented
bool motionDisabled = false;
bool isDetecting = false;
int motionTriggers = 0;

bool capture_still();
bool motion_detect();
void update_frame();

void print_frame(uint16_t frame[H][W]);
void print_state(uint16_t, uint16_t, bool);


bool motionLoop() {
    bool motionDetected = false;

    if (!motionDisabled) {
        isDetecting = true;
        if (!capture_still()) {
            loggerln("Failed motion capture");
        } else {
            motionDetected = motion_detect();
            update_frame();
        }
    }
    isDetecting = false;
    return motionDetected;
}

void motionSettings() {
    sensor_t * sensor = esp_camera_sensor_get();
    if (sensor->status.framesize != MOTION_FRAME || sensor->pixformat != PIXFORMAT_GRAYSCALE) {
        sensor_t * sensor = esp_camera_sensor_get();
        sensor->set_pixformat(sensor, PIXFORMAT_GRAYSCALE);
        sensor->set_framesize(sensor, MOTION_FRAME);
        loggerln("Motion Settings Enabled");
    }
}

void enableMotion(int level=0) {
    if (motionDisabled) {
        motionTriggers = level;
        motionDisabled = false;
        motionSettings();
    }
}

void disableMotion() {
    motionDisabled = true;
}

bool capture_still() {
    motionSettings();

    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb) return false;

    if (fb) {
        current_frame[H][W] = { 0 };
        uint32_t totalPixelTemp = 0;
        // down-sample image in blocks
        for (uint32_t i = 0; i < WIDTH * HEIGHT; i++) {
            const uint16_t x = i % WIDTH;
            const uint16_t y = floor(i / WIDTH);
            const uint8_t block_x = floor(x / BLOCK_SIZE);
            const uint8_t block_y = floor(y / BLOCK_SIZE);
            const uint8_t pixel = fb->buf[i];
            const uint16_t current = current_frame[block_y][block_x];

            // average pixels in block (accumulate)
            current_frame[block_y][block_x] += pixel;
            totalPixelTemp += pixel;
        }
        esp_camera_fb_return(fb);

        averagePixelTemp = totalPixelTemp / (WIDTH * HEIGHT);

        // average pixels in block (rescale)
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++)
                current_frame[y][x] /= BLOCK_SIZE * BLOCK_SIZE;

#if MOTION_DEBUG
        loggerln("Current frame:");
        print_frame(current_frame);
#endif
    } else return false;
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
                    logger("diff\t");
                    logger(y);
                    logger('\t');
                    loggerln(x);
                }
            }
        }
    }

    bool motion_detected = (1.0 * changes / blocks) > IMAGE_DIFF_THRESHOLD;

    if (MOTION_DEBUG || motion_detected) {
        if (motion_detected && (time(NULL) - bootTime >= 60)) motionTriggers += 1;
        print_state(changes, blocks, motion_detected);
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

void print_state(uint16_t changes, uint16_t blocks, bool motionDetected) {
    String label = "";
    String triggers = "";
    if (motionDetected) {
        label = "Motion Detected: ";
        triggers = "Triggered " + (String)motionTriggers;
    }

    logger("========== ");
    logger(label);
    logger(changes);
    logger(" out of ");
    logger(blocks);
    logger(" Blocks Changes ========== ");
    loggerln(triggers);
}

/**
 * For serial debugging
 * @param frame
 */
void print_frame(uint16_t frame[H][W]) {
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            logger(frame[y][x]);
            logger('\t');
        }

        loggerln("");
    }
}