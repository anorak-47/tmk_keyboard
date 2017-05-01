
#include "backlight/issi/is31fl3733_91tkl.h"
#include "backlight/issi/is31fl3733_twi.h"
#include "backlight/issi/is31fl3733_sdb.h"
#include "backlight/sector/sector_control.h"
#include "backlight/animations/animation.h"
#include "backlight/animations/animation_utils.h"
#include "backlight/key_led_map.h"
#include "twi/avr315/TWI_Master.h"
#include "keymap.h"
#include "action_layer.h"
#include "bootloader.h"
#include "config.h"
#include "virtser.h"
#include "util.h"
#include "crc8.h"
#include "utils.h"
#include "nfo_led.h"
#include "timer.h"
#include "host.h"
#include "virt_ser_rpc.h"
#include "eeconfig.h"
#include "keyboard.h"
#include "keycode.h"
#include "keymap.h"
#include "backlight.h"
#include "keyboard_layout_json.h"
#include "backlight/eeconfig_backlight.h"
#include "../../tmk_core/common/avr/xprintf.h"
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_VIRTSER
#include "debug.h"
#else
#include "nodebug.h"
#endif

#define DATAGRAM_START 0x02
#define DATAGRAM_STOP 0x03

#define DATAGRAM_USER_START '!'
#define DATAGRAM_USER_STOP '\n'

#define DATAGRAM_CMD_SET_PWM_MODE 0x51
#define DATAGRAM_CMD_SET_PWM_ROW 0x52
#define DATAGRAM_CMD_UPDATE_PWM 0x53
#define DATAGRAM_CMD_SAVE_PWM 0x54
#define DATAGRAM_CMD_GET_PWM_MODE 0x55
#define DATAGRAM_CMD_GET_PWM_ROW 0x56

#define MAX_MSG_LENGTH 32

#define vserprint(s) xfprintf(&virtser_send, s)
#define vserprintf(fmt, ...) xfprintf(&virtser_send, fmt, ##__VA_ARGS__)
#define vserprintfln(fmt, ...) xfprintf(&virtser_send, fmt "\n", ##__VA_ARGS__)
#define vserprintln(s) xfprintf(&virtser_send, s "\n")

#undef print
#define print(s) vserprintf(s)
#undef print_dec
#define print_dec(i) vserprintf("%u", i)
#undef print_hex8
#define print_hex8(i) vserprintf("%02X", i)

uint16_t last_receive_ts = 0;
uint8_t recv_buffer[MAX_MSG_LENGTH];

enum recvStatus
{
    recvStatusIdle = 0,
    recvStatusFoundStart = 1,
    recvStatusFoundUserStart = 2,
    recvStatusRecvPayload = 3,
    recvStatusFindStop = 4
};

enum recvStatus recv_status = recvStatusIdle;

struct user_command_t
{
    char *cmd;
    bool (*pfn_command)(uint8_t argc, char **argv);
    char *help_args;
    char *help_msg;
};

typedef struct user_command_t user_command;

bool cmd_user_help(uint8_t argc, char **argv);
bool cmd_user_info(uint8_t argc, char **argv);
bool cmd_user_ram(uint8_t argc, char **argv);
bool cmd_user_dump_eeprom(uint8_t argc, char **argv);
bool cmd_user_init_issi(uint8_t argc, char **argv);
bool cmd_user_led(uint8_t argc, char **argv);
bool cmd_user_pwm(uint8_t argc, char **argv);
bool cmd_user_rgb(uint8_t argc, char **argv);
bool cmd_user_hsv(uint8_t argc, char **argv);
bool cmd_user_sector(uint8_t argc, char **argv);
bool cmd_user_debug_config(uint8_t argc, char **argv);
bool cmd_user_keymap_config(uint8_t argc, char **argv);
bool cmd_user_bootloader_jump(uint8_t argc, char **argv);
bool cmd_user_eeprom_clear(uint8_t argc, char **argv);
bool cmd_user_backlight_eeprom_clear(uint8_t argc, char **argv);
bool cmd_user_default_layer(uint8_t argc, char **argv);
bool cmd_user_keymap_json(uint8_t argc, char **argv);
bool cmd_user_test_issi(uint8_t argc, char **argv);

const user_command user_command_table[] PROGMEM = {
    {"help", &cmd_user_help, "", "show help"},
    {"h", &cmd_user_help, "", "show help"},
    {"?", &cmd_user_help, "", "show help"},
    {"info", &cmd_user_info, "", "show info"},
    {"ram", &cmd_user_ram, "", "show free ram"},
    {"eeprom", &cmd_user_dump_eeprom, "", "dump eeprom"},
    {"dfeeprom", &cmd_user_eeprom_clear, "", "clear eeprom"},
    {"bleeprom", &cmd_user_backlight_eeprom_clear, "", "clear backlight eeprom"},
    {"sector", &cmd_user_sector, "selected [on [h s v]]", "set sector"},
    {"animation", &cmd_user_sector, "selected [on [c h s v]]", "set animation"},
    {"debug", &cmd_user_debug_config, "", "debug configuration"},
    {"keymap", &cmd_user_keymap_config, "", "keymap configuration"},
    {"keymap_json", &cmd_user_keymap_json, "", "keymap json"},
    {"layer", &cmd_user_default_layer, "", "default layer"},
    {"bootloader", &cmd_user_bootloader_jump, "", "jump to bootloader"},
    {"issi", &cmd_user_test_issi, "", "test issi"},
    {"led", &cmd_user_led, "dev cs sw on", "enable/disable led"},
    {"pwm", &cmd_user_pwm, "dev cs sw bri", "set pwm"},
    {"rgb", &cmd_user_rgb, "dev row col r g b", "set rgb"},
    {"hsv", &cmd_user_hsv, "dev row col h s v", "set hsv"},
    {"", 0, "", ""}};

static IS31FL3733_RGB device_rgb_upper;
static IS31FL3733_RGB device_rgb_lower;

static IS31FL3733 device_upper;
static IS31FL3733 device_lower;

void dump_led_buffer(IS31FL3733 *device)
{
    uint8_t *led = is31fl3733_led_buffer(device);
    vserprintf("led buffer");
    for (uint8_t sw = 0; sw < IS31FL3733_SW; ++sw)
    {
        vserprintf("%u: ");
        for (uint8_t cs = 0; cs < (IS31FL3733_CS / 8); ++cs)
        {
            vserprintf("%X ", led[sw * (IS31FL3733_CS / 8) + cs]);
        }
        vserprintf("\n");
    }
}

void dump_args(uint8_t argc, char **argv)
{
    for (uint8_t i = 0; i < argc; i++)
        vserprintf("%s ", argv[i]);
    vserprintf("\n");
}

bool cmd_user_test_issi(uint8_t argc, char **argv)
{
    if (argc == 1 && strcmp_P(PSTR("detect"), argv[0]) == 0)
    {
        unsigned char slave_address;
        unsigned char device_present;

        slave_address = IS31FL3733_I2C_ADDR(ADDR_GND, ADDR_GND);
        vserprintf("upper device at 0x%X (GND/GND): ", slave_address);
        device_present = TWI_detect(slave_address);
        vserprintfln("%u", device_present);

        slave_address = IS31FL3733_I2C_ADDR(ADDR_GND, ADDR_VCC);
        vserprintf("lower device at 0x%X (GND/VCC): ", slave_address);
        device_present = TWI_detect(slave_address);
        vserprintfln("%u", device_present);
    }
    else if (argc == 2 && strcmp_P(PSTR("init"), argv[0]) == 0)
    {
        if (atoi(argv[1]) == 0)
        {
            vserprintfln("init lower issi");
            issi.lower = &device_rgb_lower;
            device_rgb_lower.device = &device_lower;

            device_lower.gcc = 128;
            device_lower.is_master = false;
            device_lower.address = IS31FL3733_I2C_ADDR(ADDR_GND, ADDR_VCC);
            device_lower.pfn_i2c_read_reg = &i2c_read_reg;
            device_lower.pfn_i2c_write_reg = &i2c_write_reg;
            device_lower.pfn_i2c_read_reg8 = &i2c_read_reg8;
            device_lower.pfn_i2c_write_reg8 = &i2c_write_reg8;
            device_lower.pfn_hardware_enable = &sdb_hardware_enable_lower;

            is31fl3733_rgb_init(issi.lower);
        }
        else
        {
            vserprintfln("init upper issi");
            issi.upper = &device_rgb_upper;
            device_rgb_upper.device = &device_upper;

            device_upper.gcc = 128;
            device_lower.is_master = true;
            device_upper.address = IS31FL3733_I2C_ADDR(ADDR_GND, ADDR_GND);
            device_upper.pfn_i2c_read_reg = &i2c_read_reg;
            device_upper.pfn_i2c_write_reg = &i2c_write_reg;
            device_upper.pfn_i2c_read_reg8 = &i2c_read_reg8;
            device_upper.pfn_i2c_write_reg8 = &i2c_write_reg8;
            device_upper.pfn_hardware_enable = &sdb_hardware_enable_upper;

            is31fl3733_rgb_init(issi.upper);
        }
    }
    else if (argc == 2 && strcmp_P(PSTR("pwm"), argv[0]) == 0)
    {
        IS31FL3733 *device = ((atoi(argv[1]) == 0) ? issi.lower->device : issi.upper->device);

        uint8_t *pwm = is31fl3733_pwm_buffer(device);

        vserprintf("pwm buffer");
        for (uint8_t sw = 0; sw < IS31FL3733_SW; ++sw)
        {
            vserprintf("%u: ");
            for (uint8_t cs = 0; cs < IS31FL3733_CS; ++cs)
            {
                vserprintf("%X ", pwm[sw * IS31FL3733_CS + cs]);
            }
            vserprintf("\n");
        }
    }
    else if (argc == 2 && strcmp_P(PSTR("led"), argv[0]) == 0)
    {
        IS31FL3733 *device = ((atoi(argv[1]) == 0) ? issi.lower->device : issi.upper->device);

        dump_led_buffer(device);
    }
    else if (argc == 2 && strcmp_P(PSTR("open"), argv[0]) == 0)
    {
        IS31FL3733 *device = ((atoi(argv[1]) == 0) ? issi.lower->device : issi.upper->device);

        is31fl3733_detect_led_open_short_states(device);
        is31fl3733_read_led_open_states(device);

        dump_led_buffer(device);
    }
    else if (argc == 2 && strcmp_P(PSTR("short"), argv[0]) == 0)
    {
        IS31FL3733 *device = ((atoi(argv[1]) == 0) ? issi.lower->device : issi.upper->device);

        is31fl3733_detect_led_open_short_states(device);
        is31fl3733_read_led_short_states(device);

        dump_led_buffer(device);
    }
    else if (argc == 3 && strcmp_P(PSTR("gcc"), argv[0]) == 0)
    {
        IS31FL3733 *device = ((atoi(argv[1]) == 0) ? issi.lower->device : issi.upper->device);

        device->gcc = atoi(argv[2]);
        vserprintfln("gcc: %u", device->gcc);
        is31fl3733_update_global_current_control(device);
    }
    else if (argc == 3 && strcmp_P(PSTR("hsd"), argv[0]) == 0)
    {
        IS31FL3733 *device = ((atoi(argv[1]) == 0) ? issi.lower->device : issi.upper->device);

        bool enable = atoi(argv[2]);
        vserprintfln("hsd: %u", enable);
        is31fl3733_hardware_shutdown(device, enable);
    }
    else if (argc == 3 && strcmp_P(PSTR("ssd"), argv[0]) == 0)
    {
        IS31FL3733 *device = ((atoi(argv[1]) == 0) ? issi.lower->device : issi.upper->device);

        bool enable = atoi(argv[2]);
        vserprintfln("ssd: %u", enable);
        is31fl3733_software_shutdown(device, enable);
    }

    return true;
}

bool cmd_user_help(uint8_t argc, char **argv)
{
    uint8_t pos = 0;
    user_command cmd;

    vserprintf("\n\t- Anorak 91tkl - virtual console - help -\n");
    vserprintf("\n\nAll commmands must start with a '!'\n\n");

    memcpy_P(&cmd, (const void *)&user_command_table[pos++], sizeof(struct user_command_t));

    while (cmd.pfn_command)
    {
        vserprintfln("%s [%s]: %s", cmd.cmd, cmd.help_args, cmd.help_msg);
        memcpy_P(&cmd, (const void *)&user_command_table[pos++], sizeof(struct user_command_t));
    }

    vserprintf("\n\n");
    return true;
}

bool cmd_user_animation(uint8_t argc, char **argv)
{
    // [[selected on] | [c h s v]]

    vserprintfln("animation: %u, running:%u", animation_current(), animation_is_running());

    if (argc == 0)
    {
        vserprintfln("\t delay:%u, duration:%u", animation.delay_in_ms, animation.duration_in_ms);
        vserprintfln("\t hsv1: h:%u, s:%u, v:%u", animation.hsv.h, animation.hsv.s, animation.hsv.v);
        vserprintfln("\t hsv2: h:%u, s:%u, v:%u", animation.hsv2.h, animation.hsv2.s, animation.hsv2.v);
        vserprintfln("\t  rgb: r:%u, g:%u, b:%u", animation.rgb.r, animation.rgb.g, animation.rgb.b);
        return true;
    }

    if (argc == 1 && strcmp_P(PSTR("save"), argv[0]) == 0)
    {
        vserprintfln("\t  save state");
        animation_save_state();
        return true;
    }

    if (argc == 1)
    {
        uint16_t delay_in_ms = atoi(argv[0]);
        animation_set_speed(delay_in_ms);
        return true;
    }

    if (argc == 2)
    {
        uint8_t selected_animation = atoi(argv[0]);
        set_animation(selected_animation);

        bool run_animation = atoi(argv[1]);
        if (run_animation)
            start_animation();
        else
            stop_animation();

        vserprintfln("animation: set:%u, running:%u", animation_current(), animation_is_running());
        return true;
    }

    if (argc >= 4)
    {
        uint8_t pos = atoi(argv[0]);

        if (pos == 0)
        {
            animation.hsv.h = atoi(argv[1]);
            animation.hsv.s = atoi(argv[2]);
            animation.hsv.v = atoi(argv[3]);
        }
        else
        {
            animation.hsv2.h = atoi(argv[1]);
            animation.hsv2.s = atoi(argv[2]);
            animation.hsv2.v = atoi(argv[3]);
        }

        vserprintfln("\t hsv1: h:%u, s:%u, v:%u", animation.hsv.h, animation.hsv.s, animation.hsv.v);
        vserprintfln("\t hsv2: h:%u, s:%u, v:%u", animation.hsv2.h, animation.hsv2.s, animation.hsv2.v);
        return true;
    }

    return false;
}

bool cmd_user_sector(uint8_t argc, char **argv)
{
    // selected [on [h s v]]

    vserprintf("sector ");

    if (argc == 0)
    {
        vserprintfln(" state:");
        for (uint8_t s = 0; s < SECTOR_MAX; ++s)
            vserprintfln("\tsector %u: enabled:%u", s, sector_is_enabled(s));
        return true;
    }

    if (argc == 1 && strcmp_P(PSTR("save"), argv[0]) == 0)
    {
        vserprintfln("\n\t  save state");
        sector_save_state();
        return true;
    }

    if (argc == 2 && strcmp_P(PSTR("map"), argv[0]) == 0)
    {
    	uint8_t custom_map = atoi(argv[1]);
        vserprintfln("\n\t  custom map %u", custom_map);
        sector_set_custom_map(custom_map);
        return true;
    }

    uint8_t selected_sector = atoi(argv[0]);
    bool selected_sector_enabled = 0;

    if (argc >= 1)
    {
        vserprintfln("%u: enabled:%u", selected_sector, sector_is_enabled(selected_sector));
    }
    if (argc >= 2)
    {
        selected_sector_enabled = atoi(argv[1]);
        sector_select(selected_sector);
        sector_set_selected(selected_sector_enabled);

        vserprintfln("%u: enable:%u, enabled:%u", selected_sector, selected_sector_enabled,
                     sector_is_enabled(selected_sector));
    }
    if (argc >= 5)
    {
        HSV hsv;

        hsv.h = atoi(argv[2]);
        hsv.s = atoi(argv[3]);
        hsv.v = atoi(argv[4]);

        sector_selected_set_hsv_color(hsv);
        is31fl3733_91tkl_update_led_pwm(&issi);

        vserprintfln("%u: set HSV: h:%u, s:%u, v:%u", selected_sector, hsv.h, hsv.s, hsv.v);
    }

    return true;
}

bool cmd_user_info(uint8_t argc, char **argv)
{
    vserprintf("\n\t- Anorak 91tkl - version -\n");
    vserprintf("DESC: " STR(DESCRIPTION) "\n");
    vserprintf("VID: " STR(VENDOR_ID) "(" STR(MANUFACTURER) ") "
                                                            "PID: " STR(PRODUCT_ID) "(" STR(
                                                                PRODUCT) ") "
                                                                         "VER: " STR(DEVICE_VER) "\n");
    vserprintf("BUILD: " STR(VERSION) " (" __TIME__ " " __DATE__ ")\n");
    return true;
}

bool cmd_user_ram(uint8_t argc, char **argv)
{
    vserprintf("free ram: %d\n", freeRam());
    return true;
}

bool cmd_user_bootloader_jump(uint8_t argc, char **argv)
{
    bootloader_jump();
    return true;
}

bool cmd_user_eeprom_clear(uint8_t argc, char **argv)
{
    vserprintfln("eeconfig clear");
    eeconfig_init();
    return true;
}

bool cmd_user_backlight_eeprom_clear(uint8_t argc, char **argv)
{
    vserprintfln("backlight eeconfig clear");
    eeconfig_backlight_init();
    return true;
}

bool cmd_user_default_layer(uint8_t argc, char **argv)
{
#ifdef BOOTMAGIC_ENABLE
    if (argc == 1 && strcmp_P(PSTR("save"), argv[0]) == 0)
    {
        if (debug_config.raw != eeconfig_read_debug())
            eeconfig_write_debug(debug_config.raw);
    }
    else if (argc == 2 && strcmp_P(PSTR("set"), argv[0]) == 0)
    {
        uint8_t default_layer = atoi(argv[1]);

        if (default_layer)
        {
            eeconfig_write_default_layer(default_layer);
            default_layer_set((uint32_t)default_layer);
        }
        else
        {
            default_layer = eeconfig_read_default_layer();
            default_layer_set((uint32_t)default_layer);
        }
    }
    else
    {
        print("default_layer: ");
        print_dec(eeconfig_read_default_layer());
        print("\n");
    }
#endif
    return true;
}

bool cmd_user_debug_config(uint8_t argc, char **argv)
{
#ifdef BOOTMAGIC_ENABLE
    if (argc == 1 && strcmp_P(PSTR("save"), argv[0]) == 0)
    {
        if (debug_config.raw != eeconfig_read_debug())
            eeconfig_write_debug(debug_config.raw);
    }
    else if (argc == 2)
    {
        if (strcmp_P(PSTR("enable"), argv[0]) == 0)
        {
            debug_config.enable = atoi(argv[1]);
        }
        else if (strcmp_P(PSTR("matrix"), argv[0]) == 0)
        {
            debug_config.matrix = atoi(argv[1]);
        }
        else if (strcmp_P(PSTR("keyboard"), argv[0]) == 0)
        {
            debug_config.keyboard = atoi(argv[1]);
        }
        else if (strcmp_P(PSTR("mouse"), argv[0]) == 0)
        {
            debug_config.mouse = atoi(argv[1]);
        }
        else if (strcmp_P(PSTR("raw"), argv[0]) == 0)
        {
            debug_config.raw = atoi(argv[1]);
        }
    }
    else
    {
        print("debug_config.raw: ");
        print_hex8(debug_config.raw);
        print("\n");
        print(".enable: ");
        print_dec(debug_config.enable);
        print("\n");
        print(".matrix: ");
        print_dec(debug_config.matrix);
        print("\n");
        print(".keyboard: ");
        print_dec(debug_config.keyboard);
        print("\n");
        print(".mouse: ");
        print_dec(debug_config.mouse);
        print("\n");
    }
#endif
    return true;
}

#ifdef BOOTMAGIC_ENABLE
extern keymap_config_t keymap_config;
#endif

bool cmd_user_keymap_config(uint8_t argc, char **argv)
{
#ifdef BOOTMAGIC_ENABLE
    if (argc == 1 && strcmp_P(PSTR("save"), argv[0]) == 0)
    {
        if (keymap_config.raw != eeconfig_read_keymap())
            eeconfig_write_keymap(keymap_config.raw);
    }
    else if (argc == 2)
    {
        if (strcmp_P(PSTR("swap_control_capslock"), argv[0]) == 0)
        {
            keymap_config.swap_control_capslock = atoi(argv[1]);
        }
        else if (strcmp_P(PSTR("capslock_to_control"), argv[0]) == 0)
        {
            keymap_config.capslock_to_control = atoi(argv[1]);
        }
        else if (strcmp_P(PSTR("swap_lalt_lgui"), argv[0]) == 0)
        {
            keymap_config.swap_lalt_lgui = atoi(argv[1]);
        }
        else if (strcmp_P(PSTR("swap_ralt_rgui"), argv[0]) == 0)
        {
            keymap_config.swap_ralt_rgui = atoi(argv[1]);
        }
        else if (strcmp_P(PSTR("no_gui"), argv[0]) == 0)
        {
            keymap_config.no_gui = atoi(argv[1]);
        }
        else if (strcmp_P(PSTR("swap_grave_esc"), argv[0]) == 0)
        {
            keymap_config.swap_grave_esc = atoi(argv[1]);
        }
        else if (strcmp_P(PSTR("swap_backslash_backspace"), argv[0]) == 0)
        {
            keymap_config.swap_backslash_backspace = atoi(argv[1]);
        }
        else if (strcmp_P(PSTR("nkro"), argv[0]) == 0)
        {
            keymap_config.nkro = atoi(argv[1]);
#ifdef NKRO_ENABLE
            keyboard_nkro = keymap_config.nkro;
#endif
        }
        else if (strcmp_P(PSTR("raw"), argv[0]) == 0)
        {
            keymap_config.raw = atoi(argv[1]);
        }
    }
    else
    {
        print("keymap_config.raw: ");
        print_hex8(keymap_config.raw);
        print("\n");
        print(".swap_control_capslock: ");
        print_dec(keymap_config.swap_control_capslock);
        print("\n");
        print(".capslock_to_control: ");
        print_dec(keymap_config.capslock_to_control);
        print("\n");
        print(".swap_lalt_lgui: ");
        print_dec(keymap_config.swap_lalt_lgui);
        print("\n");
        print(".swap_ralt_rgui: ");
        print_dec(keymap_config.swap_ralt_rgui);
        print("\n");
        print(".no_gui: ");
        print_dec(keymap_config.no_gui);
        print("\n");
        print(".swap_grave_esc: ");
        print_dec(keymap_config.swap_grave_esc);
        print("\n");
        print(".swap_backslash_backspace: ");
        print_dec(keymap_config.swap_backslash_backspace);
        print("\n");
        print(".nkro: ");
        print_dec(keymap_config.nkro);
        print("\n");
    }
#endif
    return true;
}

bool cmd_user_dump_eeprom(uint8_t argc, char **argv)
{
#ifdef BOOTMAGIC_ENABLE
    print("default_layer: ");
    print_dec(eeconfig_read_default_layer());
    print("\n");

    debug_config_t dc;
    dc.raw = eeconfig_read_debug();
    print("debug_config.raw: ");
    print_hex8(dc.raw);
    print("\n");
    print(".enable: ");
    print_dec(dc.enable);
    print("\n");
    print(".matrix: ");
    print_dec(dc.matrix);
    print("\n");
    print(".keyboard: ");
    print_dec(dc.keyboard);
    print("\n");
    print(".mouse: ");
    print_dec(dc.mouse);
    print("\n");

    keymap_config_t kc;
    kc.raw = eeconfig_read_keymap();
    print("keymap_config.raw: ");
    print_hex8(kc.raw);
    print("\n");
    print(".swap_control_capslock: ");
    print_dec(kc.swap_control_capslock);
    print("\n");
    print(".capslock_to_control: ");
    print_dec(kc.capslock_to_control);
    print("\n");
    print(".swap_lalt_lgui: ");
    print_dec(kc.swap_lalt_lgui);
    print("\n");
    print(".swap_ralt_rgui: ");
    print_dec(kc.swap_ralt_rgui);
    print("\n");
    print(".no_gui: ");
    print_dec(kc.no_gui);
    print("\n");
    print(".swap_grave_esc: ");
    print_dec(kc.swap_grave_esc);
    print("\n");
    print(".swap_backslash_backspace: ");
    print_dec(kc.swap_backslash_backspace);
    print("\n");
    print(".nkro: ");
    print_dec(kc.nkro);
    print("\n");

#ifdef BACKLIGHT_ENABLE
    backlight_config_t bc;
    bc.raw = eeconfig_read_backlight();
    print("backlight_config.raw: ");
    print_hex8(bc.raw);
    print("\n");
    print(".enable: ");
    print_dec(bc.enable);
    print("\n");
    print(".level: ");
    print_dec(bc.level);
    print("\n");
#endif
#endif
    return true;
}

bool cmd_user_init_issi(uint8_t argc, char **argv)
{
    vserprintfln("init is31fl3733_91tkl");
    is31fl3733_91tkl_init(&issi);
    is31fl3733_91tkl_dump(&issi);
    return true;
}

bool cmd_user_led(uint8_t argc, char **argv)
{
    if (argc != 4)
        return false;

    uint8_t dev = atoi(argv[0]);
    uint8_t cs = atoi(argv[1]);
    uint8_t sw = atoi(argv[2]);
    bool on = atoi(argv[3]);

    vserprintfln("set led: dev:%u, cs:%u, sw:%u, on:%u", dev, cs, sw, on);

    IS31FL3733 *device = (dev == 0 ? issi.lower->device : issi.upper->device);

    is31fl3733_set_led(device, cs, sw, on);
    is31fl3733_update_led_enable(device);

    return true;
}

bool cmd_user_pwm(uint8_t argc, char **argv)
{
    if (argc != 4)
        return false;

    uint8_t dev = atoi(argv[0]);
    uint8_t cs = atoi(argv[1]);
    uint8_t sw = atoi(argv[2]);
    uint8_t pwm = atoi(argv[3]);

    vserprintfln("set pwm: dev:%u, cs:%u, sw:%u, pwm:%u", dev, cs, sw, pwm);

    IS31FL3733 *device = (dev == 0 ? issi.lower->device : issi.upper->device);

    is31fl3733_set_pwm(device, cs, sw, pwm);
    is31fl3733_update_led_pwm(device);

    return true;
}

bool cmd_user_rgb(uint8_t argc, char **argv)
{
    if (argc != 6)
        return false;

    RGB rgb;

    uint8_t dev = atoi(argv[0]);
    uint8_t row = atoi(argv[1]);
    uint8_t col = atoi(argv[2]);
    rgb.r = atoi(argv[3]);
    rgb.g = atoi(argv[4]);
    rgb.b = atoi(argv[5]);

    vserprintfln("set RGB: dev:%u, r:%u, c:%u, r:%u, g:%u, b:%u", dev, row, col, rgb.r, rgb.g, rgb.b);

    IS31FL3733_RGB *device = (dev == 0 ? issi.lower : issi.upper);

    is31fl3733_rgb_set_pwm(device, row, col, rgb);
    is31fl3733_update_led_pwm(device->device);

    return true;
}

bool cmd_user_hsv(uint8_t argc, char **argv)
{
    if (argc != 6)
        return false;

    HSV hsv;

    uint8_t dev = atoi(argv[0]);
    uint8_t row = atoi(argv[1]);
    uint8_t col = atoi(argv[2]);
    hsv.h = atoi(argv[3]);
    hsv.s = atoi(argv[4]);
    hsv.v = atoi(argv[5]);

    vserprintfln("set HSV: dev:%u, r:%u, c:%u, h:%u, s:%u, v:%u", dev, row, col, hsv.h, hsv.s, hsv.v);

    IS31FL3733_RGB *device = (dev == 0 ? issi.lower : issi.upper);

    is31fl3733_hsv_set_pwm(device, row, col, hsv);
    is31fl3733_update_led_pwm(device->device);

    return true;
}

bool cmd_user_keymap_json(uint8_t argc, char **argv)
{
    //__xprintf(&virtser_send, keyboard_layout_json);
    return true;
}

bool cmd_user_keymap_led_map(uint8_t argc, char **argv)
{
    uint8_t row;
    uint8_t col;
    uint8_t dev;

    vserprint("[");

    for (uint8_t key_row = 0; key_row < MATRIX_ROWS; key_row++)
    {
        vserprint("[");

        for (uint8_t key_col = 0; key_col < MATRIX_COLS; key_col++)
        {
            if (getLedPosByMatrixKey(key_row, key_col, &dev, &row, &col))
            {
                vserprintf("{kr:%u, kc:%u, d:%u, r:%u, c:%u},", key_row, key_col, dev, row, col);
            }
            else
            {
                vserprintf("{kr:%u, kc:%u, d:255, r:255, c:255},", key_row, key_col);
            }
        }

        vserprint("],");
    }

    vserprintln("]");

    return true;
}


void virtser_send_data(uint8_t const *data, uint8_t length)
{
    // dprintf("send ");
    // dump_buffer(data, length);

    for (uint8_t i = 0; i < length; ++i)
        virtser_send(*data++);
}

#if 0
/*
        %%       - print '%'
        %c       - character
        %s       - string
        %d, %u   - decimal integer
        %x, %X   - hex integer
*/

int virtser_printf(const char *fmt, ...)
{
    static char buffer[VIRT_SER_PRINTF_BUFFER_SIZE];

    int ret;
    va_list va;
    va_start(va, fmt);
    ret = mini_vsnprintf(buffer, VIRT_SER_PRINTF_BUFFER_SIZE, fmt, va);
    va_end(va);

    virtser_send_data(buffer, ret);

    return ret;
}

int virtser_printf_P(const char *fmt, ...)
{
    static char buffer[VIRT_SER_PRINTF_BUFFER_SIZE];
    static char fbuffer[32];

    strcpy_P(fbuffer, fmt);

    int ret;
    va_list va;
    va_start(va, fmt);
    ret = mini_vsnprintf(buffer, VIRT_SER_PRINTF_BUFFER_SIZE, fbuffer, va);
    va_end(va);

    virtser_send_data(buffer, ret);

    return ret;
}

int virtser_print(const char *s)
{
    int ret = 0;

    while (*s)
    {
        uart_putc(*s++);
        ret++;
    }

    return ret;
}

int virtser_print_P(const char *progmem_s)
{
    int ret = 0;
    register char c;

    while ((c = pgm_read_byte(progmem_s++)))
    {
        virtser_send(c);
        ret++;
    }

    return ret;
}
#endif

void interpret_user_command(uint8_t *buffer, uint8_t length)
{
    uint8_t pos = 0;
    char *str = (char *)buffer;
    char *token;
    char *command;
    uint8_t argc = 0;
    char *argv[10];
    struct user_command_t user_command;

    command = strsep(&str, " ");
    dprintf("user_command: %s\n", command);

    while ((token = strsep(&str, " ")))
    {
        argv[argc] = token;
        argc++;
    }

    memcpy_P(&user_command, &user_command_table[pos++], sizeof(struct user_command_t));

    while (user_command.pfn_command)
    {
        if (strcmp(user_command.cmd, command))
        {
            vserprintfln("exec: %s", command);
            dump_args(argc, argv);

            bool success = user_command.pfn_command(argc, argv);
            if (success)
            {
                vserprintfln("OK");
            }
            else
            {
                vserprintfln("cmd failed!");
            }
        }

        memcpy_P(&user_command, &user_command_table[pos++], sizeof(struct user_command_t));
    }
}

void command_set_pwm_row(uint8_t const *buffer, uint8_t length)
{
    struct cmd_set_pwm_row
    {
        // start + len + cmd + payload + crc8
        uint8_t start;
        uint8_t len;
        uint8_t cmd;
        uint8_t dev;
        uint8_t row;
        uint8_t pwm[IS31FL3733_CS];
    };

    struct cmd_set_pwm_row *cmd = (struct cmd_set_pwm_row *)buffer;

    if (cmd->len != IS31FL3733_CS + 3)
    {
        dprintf("invalid length: %d\n", cmd->len);
        return;
    }

    IS31FL3733 *device = (cmd->dev == 0 ? issi.lower->device : issi.upper->device);
    uint8_t *pwm_buffer = is31fl3733_pwm_buffer(device);
    memcpy(pwm_buffer+(cmd->row * IS31FL3733_CS), cmd->pwm, IS31FL3733_CS);
}

void command_update_pwm_row(uint8_t const *buffer, uint8_t length)
{
    struct cmd_update_pwm_row
    {
        // start + len + cmd + payload + crc8
        uint8_t start;
        uint8_t len;
        uint8_t cmd;
        uint8_t dev;
    };

    struct cmd_update_pwm_row *cmd = (struct cmd_update_pwm_row *)buffer;

    if (cmd->len != 2)
    {
        dprintf("invalid length: %d\n", cmd->len);
        return;
    }

    IS31FL3733 *device = (cmd->dev == 0 ? issi.lower->device : issi.upper->device);
    is31fl3733_update_led_pwm(device);
}

void command_set_pwm_mode(uint8_t const *buffer, uint8_t length)
{
    struct cmd_set_pwm_row
    {
        // start + len + cmd + payload + crc8
        uint8_t start;
        uint8_t len;
        uint8_t cmd;
        uint8_t map;
    };

    struct cmd_set_pwm_row *cmd = (struct cmd_set_pwm_row *)buffer;

    if (cmd->len != 2)
    {
        dprintf("invalid length: %d\n", cmd->len);
        return;
    }

    vserprintfln("\n\t  custom map %u", cmd->map);
    sector_set_custom_map(cmd->map);
}

void command_get_pwm_mode(uint8_t const *buffer, uint8_t length)
{
    struct cmd_get_pwm_mode
    {
        // start + len + cmd + payload + crc8
        uint8_t start;
        uint8_t len;
        uint8_t cmd;
        uint8_t map;
        uint8_t crc;
        uint8_t stop;
    };

    struct cmd_get_pwm_mode *cmd = (struct cmd_get_pwm_mode *)recv_buffer;

    uint8_t payload_length = sizeof(struct cmd_get_pwm_mode) - 2;

    cmd->start = DATAGRAM_START;
    cmd->stop = DATAGRAM_START;
    cmd->cmd = DATAGRAM_CMD_GET_PWM_MODE;
    cmd->len = payload_length;
    cmd->map = sector_get_custom_map();
    cmd->crc = crc8_calc(recv_buffer, 0x2D, payload_length);

    virtser_send_data(recv_buffer, sizeof(struct cmd_get_pwm_mode));
}

void command_get_pwm_row(uint8_t const *buffer, uint8_t length)
{
    struct cmd_get_pwm_row
    {
        // start + len + cmd + payload + crc8
        uint8_t start;
        uint8_t len;
        uint8_t cmd;
        uint8_t dev;
        uint8_t row;
        uint8_t pwm[IS31FL3733_CS];
        uint8_t crc;
        uint8_t stop;
    };

    struct cmd_get_pwm_row *cmd = (struct cmd_get_pwm_row *)recv_buffer;

    IS31FL3733 *device = (cmd->dev == 0 ? issi.lower->device : issi.upper->device);
    uint8_t *pwm_buffer = is31fl3733_pwm_buffer(device);
    memcpy(cmd->pwm, pwm_buffer+(cmd->row * IS31FL3733_CS), IS31FL3733_CS);

    uint8_t payload_length = sizeof(struct cmd_get_pwm_row) - 4;

    cmd->start = DATAGRAM_START;
    cmd->stop = DATAGRAM_START;
    cmd->cmd = DATAGRAM_CMD_GET_PWM_MODE;
    cmd->len = payload_length;
    cmd->crc = crc8_calc(recv_buffer, 0x2D, payload_length);

    virtser_send_data(recv_buffer, sizeof(struct cmd_get_pwm_row));
}

void command_save_pwm(uint8_t const *buffer, uint8_t length)
{
	sector_save_custom_pwm_map();
}

void command_update_pwm(uint8_t const *buffer, uint8_t length)
{
	//sector_save_custom_pwm_map();
}

uint8_t get_datagram_cmd(uint8_t const *buffer)
{
    return buffer[2];
}

void interpret_command(uint8_t const *buffer, uint8_t length)
{
    uint8_t cmd = get_datagram_cmd(buffer);
    dprintf("cmd: %x %c, l:%u\n", cmd, cmd, length);

    if (cmd == DATAGRAM_CMD_SET_PWM_ROW)
    {
        dprintf("recv set_pwm_row\n");
        command_set_pwm_row(buffer, length);
    }
    else if (cmd == DATAGRAM_CMD_UPDATE_PWM)
    {
        dprintf("recv update_pwm\n");
        command_update_pwm_row(buffer, length);
    }
    else if (cmd == DATAGRAM_CMD_SET_PWM_MODE)
    {
        dprintf("recv set_pwm_mode\n");
        command_set_pwm_mode(buffer, length);
    }
    else if (cmd == DATAGRAM_CMD_SAVE_PWM)
    {
        dprintf("recv save_pwm\n");
        command_save_pwm(buffer, length);
    }
    if (cmd == DATAGRAM_CMD_GET_PWM_ROW)
    {
        dprintf("recv get_pwm_row\n");
        command_get_pwm_row(buffer, length);
    }
    else if (cmd == DATAGRAM_CMD_GET_PWM_MODE)
    {
        dprintf("recv get_pwm_mode\n");
        command_get_pwm_mode(buffer, length);
    }
    else
    {
    	dprintf("recv unknown cmd: %u\n", cmd);
    }
}

void dump_buffer(uint8_t const *buffer, uint8_t length)
{
    dprintf("[");
    for (uint8_t i = 0; i < length; ++i)
        dprintf("%02X ", buffer[i]);
    dprintf("]\n");
}

bool check_crc(uint8_t const *buffer, uint8_t length)
{
    uint8_t crc = crc8_calc(buffer, 0x2D, length - 1);

    if (crc != buffer[length - 1])
    {
        dprintf("crc error: [%X] [%X]\n", crc, buffer[length - 1]);
        dprint("msg: ");
        dump_buffer(buffer, length);
        return false;
    }

    return true;
}

bool datagram_is_valid(uint8_t buffer_pos, uint8_t expected_length)
{
    return (buffer_pos == expected_length && check_crc(recv_buffer, buffer_pos));
}

bool datagram_is_data_start(uint8_t ucData)
{
    return (ucData == DATAGRAM_START && recv_status == recvStatusIdle);
}

bool datagram_is_userdata_start(uint8_t ucData)
{
    return (ucData == DATAGRAM_USER_START && recv_status == recvStatusIdle);
}

void virtser_recv_test(uint8_t ucData)
{
    static uint8_t buffer_pos = 0;
    static uint8_t expected_length = 0;

    last_receive_ts = timer_read();

    LedInfo2_On();
    dprintf("recv: [%02X] s:%u\n", ucData, recv_status);

    if (datagram_is_data_start(ucData))
    {
        dprintf("recv: start\n");

        buffer_pos = 0;

        recv_buffer[buffer_pos] = ucData;
        buffer_pos++;

        recv_status = recvStatusFoundStart;
    }
    else if (datagram_is_userdata_start(ucData))
    {
        dprintf("recv: user start\n");

        buffer_pos = 0;

        // do not store the user command start byte
        // recv_buffer[buffer_pos] = ucData;
        // buffer_pos++;

        recv_status = recvStatusFoundUserStart;
    }

    else if (recv_status == recvStatusFoundStart)
    {
        // start + len + cmd + payload + crc
        expected_length = ucData + 4;

        dprintf("recv: expected len %u\n", expected_length);

        recv_buffer[buffer_pos] = ucData;
        buffer_pos++;

        recv_status = recvStatusRecvPayload;

        if (expected_length >= MAX_MSG_LENGTH)
        {
            // bail out
            buffer_pos = 0;
            recv_status = recvStatusIdle;
            print("recv: payload too long\n");
        }
    }
    else if (recv_status == recvStatusFoundUserStart)
    {
        if (ucData == DATAGRAM_USER_STOP)
        {
            interpret_user_command(recv_buffer, buffer_pos);

            buffer_pos = 0;
            recv_status = recvStatusIdle;
        }

        if (buffer_pos + 1 >= MAX_MSG_LENGTH)
        {
            // bail out
            buffer_pos = 0;
            recv_status = recvStatusIdle;
            print("recv: payload too long\n");
        }

        if (ucData != DATAGRAM_USER_STOP)
        {
            recv_buffer[buffer_pos] = ucData;
            buffer_pos++;
        }
    }

    else if (recv_status == recvStatusRecvPayload && buffer_pos < expected_length)
    {
        dprintf("recv: bpos %u\n", buffer_pos);

        recv_buffer[buffer_pos] = ucData;
        buffer_pos++;

        if (buffer_pos >= expected_length)
            recv_status = recvStatusFindStop;
    }

    else if (recv_status == recvStatusFindStop && ucData == DATAGRAM_STOP)
    {
        dprintf("recv: stop %u\n", buffer_pos);

        if (datagram_is_valid(buffer_pos, expected_length))
        {
            dprintf("recv: ");
            dump_buffer(recv_buffer, buffer_pos);
            interpret_command(recv_buffer, buffer_pos);
        }
        else
        {
            print("recv: crc invalid: ");
            dump_buffer(recv_buffer, buffer_pos);
        }

        buffer_pos = 0;
        recv_status = recvStatusIdle;
    }

    if ((recv_status == recvStatusRecvPayload || recv_status == recvStatusFindStop) &&
        (buffer_pos > expected_length || buffer_pos >= MAX_MSG_LENGTH))
    {
        buffer_pos = 0;
        expected_length = 0;
        recv_status = recvStatusIdle;
        print("recv: msg invalid: ");
        dump_buffer(recv_buffer, buffer_pos);
    }

    LedInfo2_Off();
}
