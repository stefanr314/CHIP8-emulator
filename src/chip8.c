#include "../include/chip8.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void *init_chip8(Chip8 *chip8, const Chip8Config *chip8_config) {
  chip8->config = *chip8_config;

  /* set arrays explicitly */
  memset(&chip8->memory, 0,
         chip8_config->ram_size); /* set just the valid fields */
  memset(&chip8->v, 0, chip8_config->gpr_count);
  memset(&chip8->stack, 0, chip8_config->stack_depth);
  memset(&chip8->screen, 0,
         (chip8_config->screen_height * chip8_config->screen_width));
  memset(&chip8->keyboard, 0, 16);
  memcpy((chip8->memory + chip8_config->font_start_addr), FONT_SPRITES,
         sizeof(FONT_SPRITES));

  chip8->pc = chip8_config->entry_point_addr;
  chip8->sp = 0;
  chip8->dt = 0;
  chip8->st = 0;

  chip8->draw_flag = 0;

  srand((unsigned)time(NULL));

  return chip8;
}

int chip8_cycle(Chip8 *chip8) {
  uint16_t instruction = 0;
  uint8_t first_nibble = 0;
  uint8_t last_byte = 0;
  uint8_t last_nibble = 0;
  uint8_t x = 0;
  uint8_t vx = 0;

  // fetch phase
  if (chip8->pc >= CHIP8_RAM_SIZE || chip8->pc < 0x200) {
    fprintf(stderr, "CPU Trap: Memory overflow. PC out of bounds: (0x%.4x)\n",
            chip8->pc);
    goto exit_routine;
  }
  instruction =
      (chip8->memory[chip8->pc] << 8) | (chip8->memory[chip8->pc + 1]);
  chip8->pc += 2;
  first_nibble = (0xf000 & instruction) >> 12;

  // printf("%.2x\n", first_nibble);

  // decode phase
  switch (first_nibble) {
  case 0:
    if (instruction == 0x00e0) /* CLEAR SCREEN */
    {
      memset((void *)&chip8->screen, 0,
             (chip8->config.screen_height * chip8->config.screen_width));
      chip8->draw_flag = 1;
    } else if (instruction == 0x00ee) /* RET */
    {
      if (chip8->sp <= 0) {
        (void)fprintf(stderr, "Stack Underflow occured.");
        goto exit_routine;
      }

      chip8->pc = chip8->stack[chip8->sp];
      chip8->sp--;
    } else {
      (void)fprintf(
          stderr, "CPU Error: Insturction for writing direct machine code not "
                  "allowed.");
      // exit the loop
      goto exit_routine;
    }
    break;
  case 1: /* JUMP */
    chip8->pc = instruction & 0x0fff;
    break;
  case 2: /* CALL */
    if (chip8->sp >= chip8->config.stack_depth - 1) {
      (void)fprintf(stderr, "Stack Overflow occured.");
      goto exit_routine;
    }

    chip8->stack[chip8->sp] = chip8->pc;
    chip8->sp++;

    chip8->pc = instruction & 0x0fff;
    break;
  case 3: /* SKIP CONDITIONALlY if Vx equals NN*/
    if (chip8->v[(0x0f00 & instruction) >> 8] == (instruction & 0x00ff)) {
      chip8->pc += 2;
    }
    break;
  case 4: /* SKIP CONDITIONALlY if Vx not equal to NN*/
    if (chip8->v[(0x0f00 & instruction) >> 8] != (instruction & 0x00ff)) {
      chip8->pc += 2;
    }
    break;
  case 5: /* SKIP CONDITIONALlY if Vx equal to Vy */
    if (chip8->v[(0x0f00 & instruction) >> 8] ==
        chip8->v[(0x00f0 & instruction) >> 4]) {
      chip8->pc += 2;
    }
    break;
  case 6: /* SET REG Vx TO const */
    chip8->v[((unsigned)instruction & 0x0f00) >> 8] =
        (uint8_t)(instruction & 0x00ff);
    break;
  case 7: /* ADD const TO Vx (NO OVERFLOW)*/
    chip8->v[((unsigned)instruction & 0x0f00) >> 8] =
        ((unsigned)chip8->v[((unsigned)instruction & 0x0f00) >> 8] +
         ((unsigned)instruction & 0x00ff)) &
        255;
    break;
  case 8:
    last_nibble = instruction & 0x000f;
    switch (last_nibble) {
    case 0: /* Set Vx to Vy */
      chip8->v[(instruction & 0x0f00) >> 8] =
          chip8->v[(instruction & 0x00f0) >> 4];
      break;
    case 1: /* BINARY OR */
      chip8->v[(instruction & 0x0f00) >> 8] |=
          chip8->v[(instruction & 0x00f0) >> 4];
      break;
    case 2: /* BINARY AND */
      chip8->v[(instruction & 0x0f00) >> 8] &=
          chip8->v[(instruction & 0x00f0) >> 4];
      break;
    case 3: /* LOGICAL XOR */
      chip8->v[(instruction & 0x0f00) >> 8] ^=
          chip8->v[(instruction & 0x00f0) >> 4];
      break;
    case 4: /* ADD Vy to Vx (with overflow) */
    {
      int summ = chip8->v[(instruction & 0x0f00) >> 8] +
                 chip8->v[(instruction & 0x00f0) >> 4];
      chip8->v[(instruction & 0x0f00) >> 8] = summ & 0xff;

      chip8->v[0xf] = 0; /* set explicitly */

      if (summ > 255) {
        chip8->v[0xf] = 1;
      }
    } break;
    case 5: /* SUBTRACT (Vx - Vy)*/
    {
      uint8_t vx = chip8->v[(instruction & 0x0f00) >> 8];
      uint8_t vy = chip8->v[(instruction & 0x00f0) >> 4];
      chip8->v[(instruction & 0x0f00) >> 8] = (uint8_t)(vx - vy);

      chip8->v[0xf] = (vx >= vy) ? 1 : 0;
      break;
    }
    case 6: /* SHIFT RIGHT 1b */
    {
      uint8_t x = (instruction & 0x0f00) >> 8;
      if (!chip8->config.shift_quirk) {
        /* NOTE: Regular behaviour of instruction on original CHIP8 (ambiguous
         * instruction) */
        chip8->v[x] = chip8->v[(instruction & 0x00f0) >> 4];
      }
      uint8_t shifted_bit = chip8->v[x] & 1;
      chip8->v[x] >>= 1;
      chip8->v[0xf] = shifted_bit ? 1 : 0;
      break;
    }
    case 7: /* SUBTRACT (Vy - Vx) */
    {
      uint8_t vx = chip8->v[(instruction & 0x0f00) >> 8];
      uint8_t vy = chip8->v[(instruction & 0x00f0) >> 4];
      chip8->v[(instruction & 0x0f00) >> 8] = (uint8_t)(vy - vx);

      chip8->v[0xf] = (vy >= vx) ? 1 : 0;
      break;
    }
    case 0xe: /* SHIFT LEFT 1b*/
    {
      uint8_t x = (instruction & 0x0f00) >> 8;
      if (!chip8->config.shift_quirk) {
        /* NOTE: Regular behaviour of instruction on original CHIP8 (ambiguous
         * instruction) */
        chip8->v[x] = chip8->v[(instruction & 0x00f0) >> 4];
      }
      uint8_t shifted_bit = chip8->v[x] & 0x80;
      chip8->v[x] <<= 1;
      chip8->v[0xf] = shifted_bit ? 1 : 0;
      break;
    }
    default:
      fprintf(stderr, "CPU Error: Unknown instruction (instruction starting "
                      "with nibble 0x8).\n");
      goto exit_routine;
    }
    break;
  case 9: /* SKIP CONDITIONALlY if Vx not equal to Vy */
    if (chip8->v[(0x0f00 & instruction) >> 8] !=
        chip8->v[(0x00f0 & instruction) >> 4]) {
      chip8->pc += 2;
    }
    break;
  case 0xa: /* SET I */
    chip8->i = instruction & 0x0fff;
    break;
  case 0xb: /* JUMP WITH OFFSET */
  {
    uint16_t jump_at = instruction & 0x0fff;
    chip8->pc = chip8->config.jump_quirk
                    ? jump_at + chip8->v[(instruction & 0x0f00) >> 8]
                    : jump_at + chip8->v[0];
    if (chip8->pc >= CHIP8_RAM_SIZE) {
      fprintf(stderr, "Out of memory error.");
      goto exit_routine;
    }
    break;
  }
  case 0xc: /* RANDOM */
    // FIXME: implement emulator PRNG
    chip8->v[(instruction & 0x0f00) >> 8] =
        (uint8_t)(rand() & (instruction & 0x00ff));
    break;
  case 0xd: /* DRAW SCREEN */
  {
    uint8_t mask = 0x80;

    uint8_t sprite_byte = chip8->memory[chip8->i];

    uint8_t x_coordinate =
        chip8->v[((instruction & 0x0f00)) >> 8] &
        (chip8->config.screen_width - 1); /* X; wrap starting point only */
    uint8_t y_coordinate =
        chip8->v[((instruction & 0x00f0)) >> 4] &
        (chip8->config.screen_height - 1); /* Y; wrap starting point only */

    uint8_t sprite_rows = instruction & 0x000f; /* N */
    uint16_t screen_row_starting_index =
        (y_coordinate * chip8->config.screen_width) + x_coordinate;

    chip8->v[0xf] = 0; /* unset explicitly */

    /* Clip overflowing pixels */
    uint8_t bottom_margin = sprite_rows;
    if (y_coordinate + sprite_rows > chip8->config.screen_height) {
      bottom_margin = chip8->config.screen_height - y_coordinate;
    }

    uint8_t right_margin = 8;
    if (chip8->config.screen_width - x_coordinate < 8) {
      right_margin = chip8->config.screen_width - x_coordinate;
    }

    /* Draw screen loop */
    for (uint8_t r = 0; r < bottom_margin; r++) {
      screen_row_starting_index =
          ((y_coordinate + r) * chip8->config.screen_width) + x_coordinate;

      mask = 0x80;
      sprite_byte = chip8->memory[(chip8->i + r) & 0x0fff];

      for (uint8_t col = 0; col < right_margin; col++) {
        bool was_set = chip8->screen[screen_row_starting_index + col];

        chip8->screen[screen_row_starting_index + col] ^=
            ((sprite_byte & mask) >> (7 - col));

        if (!chip8->v[0xf] && was_set &&
            !chip8->screen[screen_row_starting_index + col]) {
          // Collision detection
          chip8->v[0xf] = 1;
        }
        mask = mask >> 1;
      }
    }

    chip8->draw_flag = 1;
    break;
  }
  case 0xe:
    last_byte = (uint8_t)instruction;
    x = (instruction & 0x0f00) >> 8;
    vx = chip8->v[x];

    if (vx > 0xf) {
      /* Invalid key press to be compared with */
      break;
    }
    if (last_byte == 0x9e) /* SKIP INST if key PRESSED*/
    {
      if (chip8->keyboard[vx]) {
        chip8->pc += 2;
      }

    } else if (last_byte == 0xa1) /* SKIP INST if key NOT PRESSED */
    {
      if (!chip8->keyboard[vx]) {
        chip8->pc += 2;
      }
    } else {
      fprintf(stderr, "CPU Error: Unknown instruction (instruction starting "
                      "with nibble 0xe).\n");
      goto exit_routine;
    }
    break;
  case 0xf:
    last_byte = (uint8_t)instruction;
    x = (instruction & 0x0f00) >> 8;
    if (last_byte == 0x07) /* SET Vx to TIMER reg*/ {

      chip8->v[x] = chip8->dt;
    } else if (last_byte == 0x15) /* SET TIMER reg to Vx */
    {
      chip8->dt = chip8->v[x];
    } else if (last_byte == 0x18) /* SET SOUND reg to Vx */
    {
      chip8->st = chip8->v[x];
    } else if (last_byte == 0x1e) /* ADD to I */
    {
      chip8->i += chip8->v[x];
      if (chip8->i >= 0x1000) {
        // NOTE: not truly COSMAC VIP property, but some machines rely on Vf to
        // be set if addition overflows.
        // None program relies on non happening of overflow.

        chip8->v[0xf] = 1;
      }
    } else if (last_byte == 0x0a) {
      puts("GET KEY (blocking)");
      // decrement pc -= 2 to simulate blocking -> polling
    } else if (last_byte == 0x29) /* I to FONT location */
    {
      uint8_t hex_char = chip8->v[x] & 0x0f;
      chip8->i = chip8->config.font_start_addr +
                 hex_char * 5; /* NOTE: 5 is hardcoded height of fonts in bytes.
                                  Only works for original chip8. */
    } else if (last_byte == 0x33) {
      puts("BINARY CODED DECIMAL CONVERSION");
    } else if (last_byte == 0x55) {
      puts("STORE MEMORY");
    } else if (last_byte == 0x65) {
      puts("LOAD MEMORY");
    } else {
      fprintf(stderr, "CPU Error: Unknown instruction (instruction starting "
                      "with nibble 0xf).\n");
      goto exit_routine;
    }
    break;
  default:
    fprintf(stderr, "CPU Error: Unknown instruction.\n");
    goto exit_routine;
  }

  return EXIT_SUCCESS;
exit_routine:
  return EXIT_FAILURE;
}
