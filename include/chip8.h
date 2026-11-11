/*
 * API for working with chip8 related functionalities.
 * */
#ifndef CHIP8_H
#define CHIP8_H 1

#include "config_chip8.h"
#include <stdint.h>

/* Type definitions */
typedef struct {
  Chip8Config config;

  uint8_t memory[MAX_RAM_SIZE];    /* RAM size */
  uint8_t v[MAX_GPR_COUNT];        /* General Purpose Registers */
  uint16_t stack[MAX_STACK_DEPTH]; /* Stack - storing addresses when calling
                                      subroutines */

  uint16_t pc; /* program counter */
  uint8_t sp;  /* stack pointer */
  uint16_t i;  /* index/immediate register */
  uint8_t dt;  /* delay timer register */
  uint8_t st;  /* sound timer register */

  uint8_t screen[MAX_DISPLAY_WIDTH * MAX_DISPLAY_HEIGHT];
} Chip8;

/* Function prototypes */

/* Function for initializing the chip8 emulator (given as first argument) with
 * proper configuration given as second argument. Explicit setting of
 * uninitialized array members is performed.
 * Returns the pointer to the
 * emulator. */
extern void *init_chip8(Chip8 *chip8, const Chip8Config *chip8_config);

/* Function for cycling through chip8 instructions stored in ram i.e. performing
 * fetch-decode-execute loop iteration.
 * Returns the status of loop iteration. */
extern int chip8_cycle(Chip8 *chip8);

#endif
