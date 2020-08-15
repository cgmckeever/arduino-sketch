#include "OV2640.h"

void OV2640::run(void)
{
    //return the frame buffer back to the driver for reuse
    if (fb) esp_camera_fb_return(fb);
    fb = esp_camera_fb_get();
}

void OV2640::runIfNeeded(void)
{
    if (!fb) run();
}

int OV2640::getWidth(void)
{
    runIfNeeded();
    return fb->width;
}

int OV2640::getHeight(void)
{
    runIfNeeded();
    return fb->height;
}

size_t OV2640::getSize(void)
{
    runIfNeeded();
    // FIXME - this shouldn't be possible but apparently the new cam board returns null sometimes?
    if (!fb) return 0;
    return fb->len;
}

uint8_t *OV2640::getfb(void)
{
    runIfNeeded();
    // FIXME - this shouldn't be possible but apparently the new cam board returns null sometimes?
    if (!fb) return NULL;

    return fb->buf;
}

esp_err_t OV2640::init(camera_config_t camConfig)
{
    esp_err_t err = esp_camera_init(&camConfig);
    if (err != ESP_OK)
    {
        printf("Camera probe failed with error 0x%x", err);
        return err;
    }
    // ESP_ERROR_CHECK(gpio_install_isr_service(0));

    return ESP_OK;
}
