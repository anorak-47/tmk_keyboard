#----------------------------------------------------------------------------
#-------- Anorak 91 tkl -----------------------------------------------------
#----------------------------------------------------------------------------
#
# On command line:
#
# make all = Make software.
#
# make clean = Clean out built project files.
#
# make coff = Convert ELF to AVR COFF.
#
# make extcoff = Convert ELF to AVR Extended COFF.
#
# make program = Download the hex file to the device.
#                Please customize your programmer settings(PROGRAM_CMD)
#
# make teensy = Download the hex file to the device, using teensy_loader_cli.
#               (must have teensy_loader_cli installed).
#
# make dfu = Download the hex file to the device, using dfu-programmer (must
#            have dfu-programmer installed).
#
# make flip = Download the hex file to the device, using Atmel FLIP (must
#             have Atmel FLIP installed).
#
# make dfu-ee = Download the eeprom file to the device, using dfu-programmer
#               (must have dfu-programmer installed).
#
# make flip-ee = Download the eeprom file to the device, using Atmel FLIP
#                (must have Atmel FLIP installed).
# make dfu-start = Restart using dfu-programmer
#                  (must have dfu-programmer installed).
#
# make debug = Start either simulavr or avarice as specified for debugging, 
#              with avr-gdb or avr-insight as the front end for debugging.
#
# make filename.s = Just compile filename.c into the assembler code only.
#
# make filename.i = Create a preprocessed source file for use in submitting
#                   bug reports to the GCC project.
#
# To rebuild project do "make clean" then "make all".
# 
# Default fuses (factory) : (E:F3, H:9B, L:5E)  -U lfuse:w:0x5e:m -U hfuse:w:0x9b:m -U efuse:w:0xf3:m 
# Used fuses              : (E:F0, H:D3, L:ED)  -U lfuse:w:0xde:m -U hfuse:w:0xd3:m -U efuse:w:0xf0:m 
#
# Flashing the fuses: 
#  avrdude -v -c stk500v2 -p usb646 -P /dev/ttySTK500 -B100 -U lfuse:w:0xde:m -U hfuse:w:0xd3:m -U efuse:w:0xf0:m
#
# Flashing the bootloader:
#  avrdude -v -c stk500v2 -p usb646 -P /dev/ttySTK500 -B0.1 -D -U flash:w:BootloaderDFU.hex:a
#
# If flashing fuses fails, try a chip erase:
#  avrdude -v -c stk500v2 -p usb646 -P /dev/ttySTK500 -B100 -e
#
# Flashing firmware using the bootloader:
# 
# dfu-programmer at90usb646 get bootloader-version
# dfu-programmer at90usb646 erase --force
# dfu-programmer at90usb646 flash anorak_91tkl_lufa.hex
# dfu-programmer at90usb646 launch --no-reset
#
# Virtual Serial Console: Enable RTS and DTR, use LF for input
#
# NOTE: Out-of-the-box at90usb works only with external 8MHz crystal!
# NOTE: Out-of-the-box at90usb works only with external 8MHz crystal!
# NOTE: Out-of-the-box at90usb works only with external 8MHz crystal!
# NOTE: Out-of-the-box at90usb works only with external 8MHz crystal!
#
#----------------------------------------------------------------------------

# Target file name (without extension).
TARGET = anorak_91tkl_lufa

# Directory common source filess exist
TMK_DIR = ../../tmk_core

# Directory keyboard dependent files exist
TARGET_DIR = .

PROGRAM_CMD = avrdude -v -c stk500v2 -p usb646 -P /dev/ttySTK500 -B 0.1 -U flash:w:anorak_91tkl_lufa.hex:a

# List C source files here. (C dependencies are automatically generated.)
SRC += \
	led.c \
	crc8.c \
	hooks.c \
	utils.c \
	matrix.c \
	command.c \
	sleep_led.c \
	keymap_common.c \
	keymap_91tkl.c \
	virt_ser_rpc.c \
	statusled_pwm.c \
	eeconfig_statusled_pwm.c \
	uart/uart.c \
	mini-snprintf.c \
	twi/avr315/TWI_Master.c \
	twi/avr315/twi_transmit_queue.c \
	jsonparser/jsmn.c \
	backlight/issi/is31fl3733.c \
	backlight/issi/is31fl3733_twi.c \
	backlight/issi/is31fl3733_sdb.c \
	backlight/issi/is31fl3733_iicrst.c \
	backlight/issi/is31fl3733_rgb.c \
	backlight/issi/is31fl3733_91tkl.c \
	backlight/sector/sector_control.c \
	backlight/sector/sector_led_masks.c \
	backlight/backlight_91tkl.c \
	backlight/eeconfig_backlight.c \
	backlight/color.c \
	backlight/key_led_map.c \
	backlight/animations/animation.c \
	backlight/animations/animation_utils.c \
	backlight/animations/sinus_lut.c \
	backlight/animations/plasma_color_lut.c \
	backlight/animations/sweep.c \
	backlight/animations/breathing.c \
	backlight/animations/type_o_matic.c \
	backlight/animations/type_o_circles.c \
	backlight/animations/type_o_raindrops.c \
	backlight/animations/color_cycle_all.c \
	backlight/animations/color_cycle_up_down.c \
	backlight/animations/color_cycle_left_right.c \
	backlight/animations/color_cycle_radial_1.c \
	backlight/animations/color_cycle_radial_2.c \
	backlight/animations/color_wave.c \
	backlight/animations/raindrops.c \
	backlight/animations/jellybean_raindrops.c \
	backlight/animations/flying_ball.c \
	backlight/animations/gradient_up_down.c \
	backlight/animations/gradient_left_right.c \
	backlight/animations/gradient_full_flicker.c \
	backlight/animations/conway.c \
	backlight/animations/floating_plasma.c \
	backlight/animations/particle_sys_flame.cpp \
	backlight/animations/arduino-particle-sys/Emitter_Fire.cpp \
	backlight/animations/arduino-particle-sys/Emitter_Fixed.cpp \
	backlight/animations/arduino-particle-sys/Emitter_Fountain.cpp \
	backlight/animations/arduino-particle-sys/Emitter_Side.cpp \
	backlight/animations/arduino-particle-sys/Emitter_Spin.cpp \
	backlight/animations/arduino-particle-sys/Particle_Attractor.cpp \
	backlight/animations/arduino-particle-sys/Particle_Bounce.cpp \
	backlight/animations/arduino-particle-sys/Particle_Fixed.cpp \
	backlight/animations/arduino-particle-sys/Particle_Std.cpp \
	backlight/animations/arduino-particle-sys/ParticleSys.cpp \
	backlight/animations/arduino-particle-sys/PartMatrix.cpp \
	$(TMK_DIR)/protocol/usb_hid/arduino-1.0.1/cores/arduino/WMath.cpp \
	$(TMK_DIR)/protocol/usb_hid/arduino-1.0.1/cores/arduino/new.cpp



CONFIG_H = config.h


# MCU name
MCU = at90usb646

# Processor frequency.
#     This will define a symbol, F_CPU, in all source code files equal to the
#     processor frequency in Hz. You can then use this symbol in your source code to
#     calculate timings. Do NOT tack on a 'UL' at the end, this will be done
#     automatically to create a 32-bit value in your source code.
#
#     This will be an integer division of F_USB below, as it is sourced by
#     F_USB after it has run through any CPU prescalers. Note that this value
#     does not *change* the processor frequency - it should merely be updated to
#     reflect the processor speed set externally so that the code can use accurate
#     software delays.
F_CPU = 16000000

#
# LUFA specific
#
# Target architecture (see library "Board Types" documentation).
ARCH = AVR8

# Input clock frequency.
#     This will define a symbol, F_USB, in all source code files equal to the
#     input clock frequency (before any prescaling is performed) in Hz. This value may
#     differ from F_CPU if prescaling is used on the latter, and is required as the
#     raw input clock is fed directly to the PLL sections of the AVR for high speed
#     clock generation for the USB and other AVR subsections. Do NOT tack on a 'UL'
#     at the end, this will be done automatically to create a 32-bit value in your
#     source code.
#
#     If no clock division is performed on the input clock inside the AVR (via the
#     CPU clock adjust registers or the clock division fuses), this will be equal to F_CPU.
F_USB = $(F_CPU)

# Interrupt driven control endpoint task(+60)
OPT_DEFS += -DINTERRUPT_CONTROL_ENDPOINT

# Boot Section Size in *bytes*
#   Teensy halfKay   512
#   Teensy++ halfKay 1024
#   Atmel DFU loader 4096	(TMK Alt Controller)
#   LUFA bootloader  8192
#   LUFA bootloader  4096
#   USBaspLoader     2048
OPT_DEFS += -DBOOTLOADER_SIZE=4096


# Build Options
#   comment out to disable the options.
#
BOOTMAGIC_ENABLE = yes      # Virtual DIP switch configuration(+1000)
EXTRAKEY_ENABLE = yes       # Audio control and System control(+450)
#CONSOLE_ENABLE = yes        # Console for debug(+400)
COMMAND_ENABLE = yes        # Commands for debug and configuration
NKRO_ENABLE = yes           # USB Nkey Rollover - not yet supported in LUFA
BACKLIGHT_ENABLE = yes     # Enable keyboard backlight functionality
SLEEP_LED_ENABLE = yes      # Breathing sleep LED during USB suspend
VIRTSER_ENABLE = yes       # Virtual Serial Interface (/dev/tty...)
#MOUSEKEY_ENABLE = yes      # Mouse keys(+4700)
STATUS_LED_PWM_ENABLE = yes
SLEEP_LED_USE_COMMON = no

#OPT_DEFS += -DNO_ACTION_TAPPING
#OPT_DEFS += -DNO_ACTION_LAYER
#OPT_DEFS += -DNO_ACTION_MACRO

# Search Path
VPATH += $(TARGET_DIR)
VPATH += $(TMK_DIR)
# Extra Serarch Pathes for arduino-particle-sys
VPATH += $(TMK_DIR)/protocol/usb_hid/arduino-1.0.1/cores/arduino
VPATH += $(TMK_DIR)/protocol/usb_hid/arduino-1.0.1/variants/standard

include $(TMK_DIR)/protocol/lufa.mk
include $(TMK_DIR)/common.mk
include $(TMK_DIR)/rules.mk


debug-on: EXTRAFLAGS += -DDEBUG
#debug-on: EXTRAFLAGS += -DDEBUG_LATE_TEST
#debug-on: EXTRAFLAGS += -DDEBUG_ISSI
#debug-on: EXTRAFLAGS += -DDEBUG_ISSI_SLOW_I2C
#debug-on: EXTRAFLAGS += -DDEBUG_I2C
#debug-on: EXTRAFLAGS += -DDEBUG_TX_QUEUE
#debug-on: EXTRAFLAGS += -DDEBUG_BACKLIGHT
#debug-on: EXTRAFLAGS += -DDEBUG_BACKLIGHT_EXTENDED
debug-on: EXTRAFLAGS += -DDEBUG_ANIMATION
#debug-on: EXTRAFLAGS += -DDEBUG_HOOKS
#debug-on: EXTRAFLAGS += -DDEBUG_VIRTSER
#debug-on: EXTRAFLAGS += -DLUFA_DEBUG_UART
#debug-on: EXTRAFLAGS += -DLUFA_DEBUG
#debug-on: EXTRAFLAGS += -DDEBUG_ACTION
debug-on: all

release-off: EXTRAFLAGS += -DNO_DEBUG -DNO_PRINT
release-off: OPT_DEFS := $(filter-out -DCONSOLE_ENABLE,$(OPT_DEFS))
release-off: all

release-on: EXTRAFLAGS += -DDEBUG
release-on: all

ifeq (yes,$(strip $(STATUS_LED_PWM_ENABLE)))
    SRC += $(COMMON_DIR)/avr/sleep_led.c
    OPT_DEFS += -DSTATUS_LED_PWM_ENABLE
	OPT_DEFS := $(filter-out -DSLEEP_LED_TIMER_1,$(OPT_DEFS))
	OPT_DEFS += -DSLEEP_LED_TIMER_3    
endif


dfu-hex: $(TARGET).hex
	@echo -n dfu-programmer: waiting
	@until dfu-programmer $(MCU) get bootloader-version > /dev/null 2>&1; do \
		echo  -n "."; \
		sleep 1; \
	done
	@echo
	dfu-programmer $(MCU) erase --force
	dfu-programmer $(MCU) flash $(TARGET).hex
	dfu-programmer $(MCU) launch --no-reset
	
dfu-launch:
	dfu-programmer $(MCU) launch --no-reset

dfu-eep: $(TARGET).hex $(TARGET).eep
	dfu-programmer $(MCU) flash --eeprom $(TARGET).eep
	dfu-programmer $(MCU) launch --no-reset
