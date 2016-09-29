#include "eeconfig.h"
#include <avr/eeprom.h>
#include <stdbool.h>
#include <stdint.h>

void eeconfig_init(void)
{
    eeprom_write_word(EECONFIG_MAGIC, EECONFIG_MAGIC_NUMBER);
    eeprom_write_byte(EECONFIG_DEBUG, 0);
    eeprom_write_byte(EECONFIG_DEFAULT_LAYER, 0);
    eeprom_write_byte(EECONFIG_KEYMAP, 0);
    eeprom_write_byte(EECONFIG_MOUSEKEY_ACCEL, 0);
#ifdef BACKLIGHT_ENABLE
    eeprom_write_byte(EECONFIG_BACKLIGHT, 0);
    eeprom_write_byte(EECONFIG_BACKLIGHT_REGIONS, 0);
    eeprom_write_byte(EECONFIG_BACKLIGHT_REGION_PWM, 0);
    eeprom_write_byte(EECONFIG_BACKLIGHT_REGION_PWM + 1, 0);
    eeprom_write_byte(EECONFIG_BACKLIGHT_REGION_PWM + 2, 0);
    eeprom_write_byte(EECONFIG_BACKLIGHT_REGION_PWM + 3, 0);
    eeprom_write_byte(EECONFIG_BACKLIGHT_REGION_PWM + 4, 0);
    eeprom_write_byte(EECONFIG_BACKLIGHT_REGION_PWM + 5, 0);
    eeprom_write_byte(EECONFIG_BACKLIGHT_REGION_PWM + 6, 0);
    eeprom_write_byte(EECONFIG_BACKLIGHT_REGION_PWM + 7, 0);
#endif
}

void eeconfig_enable(void)
{
    eeprom_write_word(EECONFIG_MAGIC, EECONFIG_MAGIC_NUMBER);
}

void eeconfig_disable(void)
{
    eeprom_write_word(EECONFIG_MAGIC, 0xFFFF);
}

bool eeconfig_is_enabled(void)
{
    return (eeprom_read_word(EECONFIG_MAGIC) == EECONFIG_MAGIC_NUMBER);
}

uint8_t eeconfig_read_debug(void)
{
    return eeprom_read_byte(EECONFIG_DEBUG);
}
void eeconfig_write_debug(uint8_t val)
{
    eeprom_write_byte(EECONFIG_DEBUG, val);
}

uint8_t eeconfig_read_default_layer(void)
{
    return eeprom_read_byte(EECONFIG_DEFAULT_LAYER);
}

void eeconfig_write_default_layer(uint8_t val)
{
    eeprom_write_byte(EECONFIG_DEFAULT_LAYER, val);
}

uint8_t eeconfig_read_keymap(void)
{
    return eeprom_read_byte(EECONFIG_KEYMAP);
}

void eeconfig_write_keymap(uint8_t val)
{
    eeprom_write_byte(EECONFIG_KEYMAP, val);
}

#ifdef BACKLIGHT_ENABLE
uint8_t eeconfig_read_backlight(void)
{
    return eeprom_read_byte(EECONFIG_BACKLIGHT);
}

void eeconfig_write_backlight(uint8_t val)
{
    eeprom_write_byte(EECONFIG_BACKLIGHT, val);
}

uint8_t eeconfig_read_backlight_regions(void)
{
    return eeprom_read_byte(EECONFIG_BACKLIGHT_REGIONS);
}

void eeconfig_write_backlight_regions(uint8_t val)
{
    eeprom_write_byte(EECONFIG_BACKLIGHT_REGIONS, val);
}

uint8_t eeconfig_read_backlight_region_brightness(uint8_t region)
{
    return eeprom_read_byte(EECONFIG_BACKLIGHT_REGION + region);
}

void eeconfig_write_backlight_region_brightness(uint8_t region, uint8_t brightness)
{
    eeprom_write_byte(EECONFIG_BACKLIGHT_REGION_PWM + region, brightness);
}
#endif
