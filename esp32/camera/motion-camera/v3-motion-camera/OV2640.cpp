#include "OV2640.h"

size_t OV2640::run(void)
{
    if (buffer) esp_camera_fb_return(buffer);
    buffer = esp_camera_fb_get();
    return buffer->len;
}

void OV2640::runIfNeeded(void)
{
    if (!buffer) run();
}

int OV2640::getWidth(void)
{
    runIfNeeded();
    return buffer->width;
}

int OV2640::getHeight(void)
{
    runIfNeeded();
    return buffer->height;
}

size_t OV2640::getSize(void)
{
    runIfNeeded();
    return (!buffer) ? 0 : buffer->len;
}

uint8_t *OV2640::getBuffer(void)
{
    runIfNeeded();
    return (!buffer) ? NULL : buffer->buf;
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
