
#include "animation_utils.h"
#include "config.h"
#include "../sector/sector_control.h"
#include <avr/pgmspace.h>

animation_interface animation;

static uint8_t* upper_leds;
static uint8_t* upper_pwm;
static uint8_t* lower_leds;
static uint8_t* lower_pwm;

void animation_prepare(bool set_all_to_black)
{
	upper_leds = (uint8_t*)malloc(IS31FL3733_LED_ENABLE_SIZE * sizeof(uint8_t));
	upper_pwm = (uint8_t*)malloc(IS31FL3733_LED_PWM_SIZE * sizeof(uint8_t));

	lower_leds = (uint8_t*)malloc(IS31FL3733_LED_ENABLE_SIZE * sizeof(uint8_t));
	lower_pwm = (uint8_t*)malloc(IS31FL3733_LED_PWM_SIZE * sizeof(uint8_t));

	memcpy(upper_leds, is31fl3733_led_buffer(issi.upper->device), IS31FL3733_LED_ENABLE_SIZE * sizeof(uint8_t));
	memcpy(upper_pwm, is31fl3733_pwm_buffer(issi.upper->device), IS31FL3733_LED_PWM_SIZE * sizeof(uint8_t));

	memcpy(lower_leds, is31fl3733_led_buffer(issi.lower->device), IS31FL3733_LED_ENABLE_SIZE * sizeof(uint8_t));
	memcpy(lower_pwm, is31fl3733_pwm_buffer(issi.lower->device), IS31FL3733_LED_PWM_SIZE * sizeof(uint8_t));

	sector_enable_all_leds();

	if (set_all_to_black)
	{
		is31fl3733_fill(issi.upper->device, 0);
		is31fl3733_fill(issi.lower->device, 0);
	}

	is31fl3733_91tkl_update_led_pwm(&issi);
	is31fl3733_91tkl_update_led_enable(&issi);
}

void animation_postpare()
{
	memcpy(is31fl3733_led_buffer(issi.upper->device), upper_leds, IS31FL3733_LED_ENABLE_SIZE * sizeof(uint8_t));
	memcpy(is31fl3733_pwm_buffer(issi.upper->device), upper_pwm, IS31FL3733_LED_PWM_SIZE * sizeof(uint8_t));

	memcpy(is31fl3733_led_buffer(issi.lower->device), lower_leds, IS31FL3733_LED_ENABLE_SIZE * sizeof(uint8_t));
	memcpy(is31fl3733_pwm_buffer(issi.lower->device), lower_pwm, IS31FL3733_LED_PWM_SIZE * sizeof(uint8_t));

	free(upper_leds);
	free(upper_pwm);

	free(lower_leds);
	free(lower_pwm);

	is31fl3733_91tkl_update_led_pwm(&issi);
	is31fl3733_91tkl_update_led_enable(&issi);
}

void draw_rgb_pixel(IS31FL3733_91TKL *device_91tkl, int16_t x, int16_t y, RGB color)
{
    uint8_t row;
    uint8_t col;
    uint8_t device_number;
    IS31FL3733 *device;

    if (x < 0 || x > MATRIX_COLS)
        return;
    if (y < 0 || y > MATRIX_ROWS)
        return;

    getLedPosByMatrixKey(x, y, &device_number, &row, &col);

    device = device_number ? device_91tkl->upper->device : device_91tkl->lower->device;

    is31fl3733_rgb_set_pwm(device, row, col, color);
}

void draw_hsv_pixel(IS31FL3733_91TKL *device_91tkl, int16_t x, int16_t y, HSV color)
{
    uint8_t row;
    uint8_t col;
    uint8_t device_number;
    IS31FL3733 *device;

    if (x < 0 || x > MATRIX_COLS)
        return;
    if (y < 0 || y > MATRIX_ROWS)
        return;

    getLedPosByMatrixKey(x, y, &device_number, &row, &col);

    device = device_number ? device_91tkl->upper->device : device_91tkl->lower->device;

    is31fl3733_hsv_set_pwm(device, row, col, color);
}
