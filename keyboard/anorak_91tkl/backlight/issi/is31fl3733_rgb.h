#ifndef _IS31FL3733_RGB_H_
#define _IS31FL3733_RGB_H_

#include "is31fl3733.h"
#include "../color.h"

/** IS31FL3733_RGB structure.
  */
struct IS31FL3733_RGB_Device
{
    IS31FL3733 *device;

    union {
        struct
        {
            uint8_t r;
            uint8_t g;
            uint8_t b;
        };
        uint8_t color[3];
    } offsets;
};

typedef struct IS31FL3733_RGB_Device IS31FL3733_RGB;

/// Init LED matrix for normal operation.
void is31fl3733_rgb_init(IS31FL3733_RGB *device);

/// Set brightness level for all enabled LEDs.
void is31fl3733_fill_rgb_masked(IS31FL3733_RGB *device, RGB color);

/// Set brightness level for one color all enabled LEDs.
void is31fl3733_fill_hsv_masked(IS31FL3733_RGB *device, HSV color);

#endif /* _IS31FL3733_RGB_H_ */
