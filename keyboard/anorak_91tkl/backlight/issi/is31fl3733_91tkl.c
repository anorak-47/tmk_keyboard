
#include "is31fl3733_91tkl.h"

#include "is31fl3733_sdb.h"
#include "is31fl3733_twi.h"

IS31FL3733_91TKL issi;

IS31FL3733_RGB device_rgb_upper;
IS31FL3733_RGB device_rgb_lower;

IS31FL3733 device_upper;
IS31FL3733 device_lower;

static bool is_initialized = false;

void is31fl3733_91tkl_init(IS31FL3733_91TKL *device)
{
    device->upper = &device_rgb_upper;
    device_rgb_upper.device = &device_upper;

    device_upper.gcc = 128;
    device_upper.address = IS31FL3733_I2C_ADDR(ADDR_GND, ADDR_GND);
    device_upper.pfn_i2c_read_reg = &i2c_read_reg;
    device_upper.pfn_i2c_write_reg = &i2c_write_reg;
    device_upper.pfn_i2c_read_reg8 = &i2c_read_reg8;
    device_upper.pfn_i2c_write_reg8 = &i2c_write_reg8;
    device_upper.pfn_hardware_enable = &sdb_hardware_enable_upper;

    is31fl3733_rgb_init(device->upper);

    device->lower = &device_rgb_lower;
    device_rgb_lower.device = &device_lower;

    device_lower.gcc = 128;
    device_lower.address = IS31FL3733_I2C_ADDR(ADDR_GND, ADDR_VCC);
    device_lower.pfn_i2c_read_reg = &i2c_read_reg;
    device_lower.pfn_i2c_write_reg = &i2c_write_reg;
    device_lower.pfn_i2c_read_reg8 = &i2c_read_reg8;
    device_lower.pfn_i2c_write_reg8 = &i2c_write_reg8;
    device_lower.pfn_hardware_enable = &sdb_hardware_enable_lower;

    is31fl3733_rgb_init(device->lower);

    is_initialized = true;
}

void is31fl3733_91tkl_hardware_shutdown(IS31FL3733_91TKL *device, bool enabled)
{
	is31fl3733_hardware_shutdown(device->upper->device, enabled);
	is31fl3733_hardware_shutdown(device->lower->device, enabled);
}

void is31fl3733_91tkl_dump(IS31FL3733_91TKL *device)
{

}

void is31fl3733_91tkl_fill_rgb_masked(IS31FL3733_91TKL *device, RGB color)
{
	is31fl3733_fill_rgb_masked(device->upper, color);
	is31fl3733_fill_rgb_masked(device->lower, color);
}

void is31fl3733_91tkl_fill_hsv_masked(IS31FL3733_91TKL *device, HSV color)
{
	is31fl3733_fill_hsv_masked(device->upper, color);
	is31fl3733_fill_hsv_masked(device->lower, color);
}

void is31fl3733_91tkl_power_target(IS31FL3733_91TKL *device, uint16_t milliampere)
{
    // TODO: set a value
    device->upper->device->gcc = 128;
    device->lower->device->gcc = 128;

    is31fl3733_update_global_configuration(device->upper->device);
    is31fl3733_update_global_configuration(device->lower->device);
}

bool is31fl3733_91tkl_initialized(void)
{
    return is_initialized;
}

void is31fl3733_91tkl_update(IS31FL3733_91TKL *device)
{
	is31fl3733_update(device->upper->device);
	is31fl3733_update(device->lower->device);
}

void is31fl3733_91tkl_update_led_enable(IS31FL3733_91TKL *device)
{
	is31fl3733_update_led_enable(device->upper->device);
	is31fl3733_update_led_enable(device->lower->device);
}

void is31fl3733_91tkl_update_led_pwm(IS31FL3733_91TKL *device)
{
	is31fl3733_update_led_pwm(device->upper->device);
	is31fl3733_update_led_pwm(device->lower->device);
}
