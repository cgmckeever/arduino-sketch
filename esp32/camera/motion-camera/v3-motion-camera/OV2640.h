/**

https://github.com/arkhipenko/esp32-cam-mjpeg-multiclient

**/

#ifndef OV2640_H_
#define OV2640_H_

#include "esp_camera.h"

class OV2640
{
public:
    OV2640() {
        buffer = NULL;
    };
    ~OV2640() {};

    esp_err_t init(camera_config_t config);
    size_t run(void);
    size_t getSize(void);
    uint8_t *getBuffer(void);
    int getWidth(void);
    int getHeight(void);

private:
    // grab a frame if we don't already have one
    void runIfNeeded();

    camera_fb_t *buffer;
};

#endif //OV2640_H_
