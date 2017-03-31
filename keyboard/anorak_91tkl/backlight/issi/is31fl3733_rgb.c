
#include "is31fl3733_rgb.h"

void is31fl3733_rgb_init(IS31FL3733_RGB *device)
{
    device->offsets.r = 0;
    device->offsets.g = 2;
    device->offsets.b = 1;

    is31fl3733_init(device->device);
}

void is31fl3733_fill_rgb_masked(IS31FL3733_RGB *device, RGB color)
{
    uint8_t mask_byte;
    uint8_t mask_bit;

    // Set brightness level of all LED's.
    for (uint8_t c = 0; c < 3; ++c)
    {
        for (uint8_t i = device->offsets.color[c]; i < IS31FL3733_LED_PWM_SIZE; i += 3)
        {
            mask_byte = i / 8;
            mask_bit = i % 8;

            if (device->device->mask[mask_byte] & (1 << mask_bit))
                device->device->pwm[i] = color.rgb[c];
        }
    }
}

void is31fl3733_fill_hsv_masked(IS31FL3733_RGB *device, HSV color)
{
    RGB color_rgb = hsv_to_rgb(color);
    is31fl3733_fill_rgb_masked(device, color_rgb);
}
