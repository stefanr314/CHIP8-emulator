/*
 *
 * API for configuring chip8 specs, depending on the machine to be emulated.
 * This API should not be included separetly. Use chip8.h instead.
 *
 * */

#ifndef CONFIG_CHIP8_H
#define CONFIG_CHIP8_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Macro definitions. */

/* CHIP8 ORIGINAL/DEFAULT SPECS. */
#define CHIP8_RAM_SIZE 4096
#define CHIP8_ENTRY_POINT_ADDRESS 0x200
#define CHIP8_MAX_ROM_SIZE (CHIP8_RAM_SIZE - CHIP8_ENTRY_POINT_ADDRESS)
#define CHIP8_STACK_DEPTH                                                      \
  12 /* varies, but on COSMAC VIP 12 was default; 16 will work too*/
#define CHIP8_GPR_COUNT 16

#define CHIP8_FONT_START_ADDRESS 0x050
#define CHIP8_FONT_SIZE_B 5

#define CHIP8_DISPLAY_WIDTH 64
#define CHIP8_DISPLAY_HEIGHT 32

#define CHIP8_TIMER_HZ 60
#define CHIP8_SIMULATION_HZ 700
#define CHIP8_DISPLAY_REFRESH_RATE 60

/* SCHIP SPECS. */
#define SCHIP_RAM_SIZE 4096
#define SCHIP_ENTRY_POINT_ADDRESS 0x200
#define SCHIP_MAX_ROM_SIZE (SCHIP_RAM_SIZE - SCHIP_ENTRY_POINT_ADDRESS)
#define SCHIP_STACK_DEPTH 16
#define SCHIP_GPR_COUNT 16

#define SCHIP_LOW_RES_DISPLAY_WIDTH 64
#define SCHIP_HIGH_RES_DISPLAY_WIDTH 128
#define SCHIP_LOW_RES_DISPLAY_HEIGHT 32
#define SCHIP_HIGH_RES_DISPLAY_HEIGHT 64

#define SCHIP_MAX_DISPLAY_WIDTH SCHIP_HIGH_RES_DISPLAY_WIDTH
#define SCHIP_MAX_DISPLAY_HEIGHT SCHIP_HIGH_RES_DISPLAY_HEIGHT

#define SCHIP_REGULAR_FONT_SIZE_B 5
#define SCHIP_BIG_FONT_SIZE_B 10

#define SCHIP_TIMER_HZ 60
#define SCHIP_SIMULATION_HZ 1500
#define SCHIP_DISPLAY_REFRESH_RATE 60

#define SCHIP_RPL_FLAGS_COUNT 8

/* ETI660 machine specs. */
#define ETI660_RAM_SIZE 2048
#define ETI660_ENTRY_POINT_ADDRESS 0x600
#define ETI660_MAX_ROM_SIZE (ETI660_RAM_SIZE - ETI660_ENTRY_POINT_ADDRESS)

/* Max values - as array constants*/
#define MAX_RAM_SIZE SCHIP_RAM_SIZE
#define MAX_GPR_COUNT SCHIP_GPR_COUNT
#define MAX_STACK_DEPTH SCHIP_STACK_DEPTH
#define MAX_DISPLAY_WIDTH SCHIP_MAX_DISPLAY_WIDTH
#define MAX_DISPLAY_HEIGHT SCHIP_MAX_DISPLAY_HEIGHT

/* Types definitions. */
typedef enum {
  ORIGINAL_CHIP8, /* COSMAC VIP original machine*/
  SCHIP,          /* SUPER-CHIP, CHIP-48 add-ons*/
  ETI660,         /* ETI600 machine */
  XO_CHIP,        /* TODO: */
} Chip8Model;

typedef struct {
  Chip8Model model;

  /* Memory & registers */
  size_t ram_size;
  size_t entry_point_addr;
  size_t max_rom_size;
  uint8_t stack_depth;
  uint8_t gpr_count;

  /* Display */
  uint16_t screen_width;
  uint16_t screen_height;

  /* Clocks */
  uint16_t simulation_hz;
  uint8_t timer_hz;
  uint8_t display_refresh_rate;

  /* Quirks (hardware differences) */
  bool shift_quirk;
  bool load_store_quirk;
  bool jump_quirk;
  bool wrap_sprites_quirk;
} Chip8Config;

/* Define presets */
static const Chip8Config CONFIG_PRESETS[] = {
    [ORIGINAL_CHIP8] = {.model = ORIGINAL_CHIP8,
                        .ram_size = CHIP8_RAM_SIZE,
                        .entry_point_addr = CHIP8_ENTRY_POINT_ADDRESS,
                        .max_rom_size = CHIP8_MAX_ROM_SIZE,
                        .stack_depth = CHIP8_STACK_DEPTH,
                        .gpr_count = CHIP8_GPR_COUNT,
                        .screen_width = CHIP8_DISPLAY_WIDTH,
                        .screen_height = CHIP8_DISPLAY_HEIGHT,
                        .simulation_hz = CHIP8_SIMULATION_HZ,
                        .timer_hz = CHIP8_TIMER_HZ,
                        .display_refresh_rate = CHIP8_DISPLAY_REFRESH_RATE,
                        .shift_quirk = 0,
                        .load_store_quirk = 0,
                        .jump_quirk = 0,
                        .wrap_sprites_quirk = 0},
    [SCHIP] = {.model = SCHIP,
               .ram_size = SCHIP_RAM_SIZE,
               .entry_point_addr = SCHIP_ENTRY_POINT_ADDRESS,
               .max_rom_size = SCHIP_MAX_ROM_SIZE,
               .stack_depth = SCHIP_STACK_DEPTH,
               .gpr_count = SCHIP_GPR_COUNT,
               .screen_width = SCHIP_MAX_DISPLAY_WIDTH,
               .screen_height = SCHIP_MAX_DISPLAY_HEIGHT,
               .simulation_hz = SCHIP_SIMULATION_HZ,
               .timer_hz = SCHIP_TIMER_HZ,
               .display_refresh_rate = SCHIP_DISPLAY_REFRESH_RATE,
               .shift_quirk = 1,
               .load_store_quirk = 1,
               .jump_quirk = 1,
               .wrap_sprites_quirk = 1},
    [ETI660] = {.model = ETI660,
                .ram_size = ETI660_RAM_SIZE,
                .entry_point_addr = ETI660_ENTRY_POINT_ADDRESS,
                .max_rom_size = ETI660_MAX_ROM_SIZE,
                .stack_depth = CHIP8_STACK_DEPTH,
                .gpr_count = CHIP8_GPR_COUNT,
                .screen_width = CHIP8_DISPLAY_WIDTH,
                .screen_height = CHIP8_DISPLAY_HEIGHT,
                .simulation_hz = CHIP8_SIMULATION_HZ,
                .timer_hz = CHIP8_TIMER_HZ,
                .display_refresh_rate = CHIP8_DISPLAY_REFRESH_RATE,
                .shift_quirk = 0,
                .load_store_quirk = 0,
                .jump_quirk = 0,
                .wrap_sprites_quirk = 0},
};
#endif
