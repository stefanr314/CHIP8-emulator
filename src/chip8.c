#include "../include/chip8.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static inline void draw_screen(Chip8 *chip8, uint16_t instruction) {

  uint8_t x_coordinate = chip8->v[((instruction & 0x0f00)) >> 8] &
                         (chip8->config.screen_width -
                          1); /* X; wrap starting point only (original CHIP8) */
  uint8_t y_coordinate = chip8->v[((instruction & 0x00f0)) >> 4] &
                         (chip8->config.screen_height -
                          1); /* Y; wrap starting point only (original CHIP8) */

  uint8_t sprite_rows = instruction & 0x000f; /* N */

  chip8->v[0xf] = 0; /* unset explicitly */

  /* Clip overflowing pixels (iff original CHIP8)*/
  uint8_t bottom_margin = sprite_rows;
  if (!chip8->config.wrap_sprites_quirk &&
      y_coordinate + sprite_rows > chip8->config.screen_height) {
    bottom_margin = (uint8_t)(chip8->config.screen_height - y_coordinate);
  }

  uint8_t right_margin = 8;
  if (!chip8->config.wrap_sprites_quirk &&
      chip8->config.screen_width - x_coordinate < 8) {
    right_margin = (uint8_t)(chip8->config.screen_width - x_coordinate);
  }

  /* Draw screen loop - consider overflows on screen buffer, since SCHIP uses
   * same loop */
  for (uint8_t r = 0; r < bottom_margin; r++) {
    uint8_t sprite_byte = chip8->memory[(chip8->i + r) & 0x0fff];

    for (uint8_t col = 0; col < right_margin; col++) {
      uint8_t py = (y_coordinate + r) & (chip8->config.screen_height - 1);
      uint8_t px = (x_coordinate + col) & (chip8->config.screen_width - 1);
      uint16_t screen_idx = (py * chip8->config.screen_width) + px;

      bool was_set = chip8->screen[screen_idx];

      chip8->screen[screen_idx] ^= ((sprite_byte >> (7 - col)) & 0x1);

      if (!chip8->v[0xf] && was_set && !chip8->screen[screen_idx]) {
        // Collision detection
        chip8->v[0xf] = 1;
      }
    }
  }

  chip8->draw_flag = 1;
}

static inline int get_key(Chip8 *chip8, uint8_t x) {
  // NOTE: trigger is KEYDOWN not KEYUP

  if (chip8->first_pressed_inx == -1) {
    for (uint8_t i = 0; i < 16; i++) {
      if (chip8->keyboard[i]) {
        chip8->first_pressed_inx = i;
        return 1;
      }
    }
  } else {
    if (!chip8->keyboard[chip8->first_pressed_inx]) {
      chip8->v[x] = (uint8_t)chip8->first_pressed_inx;
      chip8->first_pressed_inx = -1;
      return 0;
    }
  }

  return 1;
}

void *init_chip8(Chip8 *chip8, const Chip8Config *chip8_config) {
  chip8->config = *chip8_config;

  /* set arrays explicitly */
  memset(&chip8->memory, 0,
         chip8_config->ram_size); /* set just the valid fields */
  memset(&chip8->v, 0, chip8_config->gpr_count);
  memset(&chip8->stack, 0,
         chip8_config->stack_depth *
             sizeof(uint16_t)); /* init whole stack in B */
  memset(&chip8->screen, 0,
         (chip8_config->screen_height * chip8_config->screen_width));
  memset(&chip8->keyboard, 0, 16);
  memcpy((chip8->memory + chip8_config->font_start_addr), FONT_SPRITES,
         sizeof(FONT_SPRITES));

  chip8->pc = (uint16_t)chip8_config->entry_point_addr;
  chip8->sp = 0; /* Point to stack address of last entry */
  chip8->dt = 0;
  chip8->st = 0;
  chip8->i = 0; /* set it explicitly */

  chip8->draw_flag = 0;
  chip8->first_pressed_inx = -1;

  srand((unsigned)time(NULL));

  return chip8;
}

int chip8_cycle(Chip8 *chip8) {
  // fetch phase
  if (chip8->pc > CHIP8_RAM_SIZE - 2 ||
      chip8->pc < chip8->config.entry_point_addr) {
    fprintf(stderr, "CPU Trap: Memory overflow. PC out of bounds: (0x%.4x)\n",
            chip8->pc);
    goto exit_routine;
  }
  uint16_t instruction = (uint16_t)(chip8->memory[chip8->pc] << 8) |
                         (chip8->memory[chip8->pc + 1]);
  chip8->pc += 2;

  uint8_t op = (0xf000 & instruction) >> 12;
  uint8_t n = instruction & 0x000f;
  uint8_t nn = instruction & 0x00ff;
  uint16_t nnn = instruction & 0x0fff;
  uint8_t x = (0x0f00 & instruction) >> 8;
  uint8_t y = (0x00f0 & instruction) >> 4;

#ifdef DEBUG
  printf("OPCODE: %#.2x\n", op);
#endif

  // decode phase
  switch (op) {
  case 0:
    if (instruction == 0x00e0) /* CLEAR SCREEN */
    {
      memset((void *)&chip8->screen, 0,
             (chip8->config.screen_height * chip8->config.screen_width));
      chip8->draw_flag = 1;
    } else if (instruction == 0x00ee) /* RET */
    {
      if (chip8->sp <= 0) {
        fprintf(stderr, "Stack Underflow occured.");
        goto exit_routine;
      }

      chip8->pc = chip8->stack[chip8->sp - 1];
      chip8->sp--;
    } else {
      fprintf(stderr,
              "CPU Error: Insturction for writing direct machine code not "
              "allowed.");
      // exit the loop
      goto exit_routine;
    }
    break;
  case 1: /* JUMP */
    chip8->pc = nnn;
    break;
  case 2: /* CALL */
    if (chip8->sp >= chip8->config.stack_depth) {
      fprintf(stderr, "Stack Overflow occured.");
      goto exit_routine;
    }

    chip8->sp++;
    chip8->stack[chip8->sp - 1] = chip8->pc;

    chip8->pc = nnn;
    break;
  case 3: /* SKIP CONDITIONALlY if Vx equals NN*/
    if (chip8->v[x] == nn) {
      chip8->pc += 2;
    }
    break;
  case 4: /* SKIP CONDITIONALlY if Vx not equal to NN*/
    if (chip8->v[x] != nn) {
      chip8->pc += 2;
    }
    break;
  case 5: /* SKIP CONDITIONALlY if Vx equal to Vy */
    if (chip8->v[x] == chip8->v[y]) {
      chip8->pc += 2;
    }
    break;
  case 6: /* SET REG Vx TO const */
    chip8->v[x] = (uint8_t)(nn);
    break;
  case 7: /* ADD const TO Vx (NO OVERFLOW)*/
    chip8->v[x] = (chip8->v[x] + nn) & 255;
    break;
  case 8:
    switch (n) {
    case 0: /* Set Vx to Vy */
      chip8->v[x] = chip8->v[y];
      break;
    case 1: /* BINARY OR */
      chip8->v[x] |= chip8->v[y];

      /* NOTE: Original COSMAC VIP would reset vf flag due to CPU's ALU way of
       * operating */
      if (chip8->config.vf_reset_quirk) {
        chip8->v[0xf] = 0;
      }
      break;
    case 2: /* BINARY AND */
      chip8->v[x] &= chip8->v[y];

      /* NOTE: Original COSMAC VIP would reset vf flag due to CPU's ALU way of
       * operating */
      if (chip8->config.vf_reset_quirk) {
        chip8->v[0xf] = 0;
      }
      break;
    case 3: /* LOGICAL XOR */
      chip8->v[x] ^= chip8->v[y];

      /* NOTE: Original COSMAC VIP would reset vf flag due to CPU's ALU way of
       * operating */
      if (chip8->config.vf_reset_quirk) {
        chip8->v[0xf] = 0;
      }
      break;
    case 4: /* ADD Vy to Vx (with overflow) */
    {
      int summ = chip8->v[x] + chip8->v[y];
      chip8->v[x] = summ & 0xff;

      chip8->v[0xf] = 0; /* set explicitly */

      if (summ > 255) {
        chip8->v[0xf] = 1;
      }
    } break;
    case 5: /* SUBTRACT (Vx - Vy)*/
    {
      uint8_t vx = chip8->v[x];
      uint8_t vy = chip8->v[y];
      chip8->v[x] = (uint8_t)(vx - vy);

      chip8->v[0xf] = (vx >= vy) ? 1 : 0;
      break;
    }
    case 6: /* SHIFT RIGHT 1b (ambiguous instruction) */
    {
      if (!chip8->config.shift_quirk) {
        /* NOTE: Regular behaviour of instruction on original CHIP8 */
        chip8->v[x] = chip8->v[y];
      }

      uint8_t shifted_bit = chip8->v[x] & 1;
      chip8->v[x] >>= 1;
      chip8->v[0xf] = shifted_bit ? 1 : 0;
      break;
    }
    case 7: /* SUBTRACT (Vy - Vx) */
    {
      uint8_t vx = chip8->v[x];
      uint8_t vy = chip8->v[y];
      chip8->v[x] = (uint8_t)(vy - vx);

      chip8->v[0xf] = (vy >= vx) ? 1 : 0;
      break;
    }
    case 0xe: /* SHIFT LEFT 1b (ambiguous instruction) */
    {
      if (!chip8->config.shift_quirk) {
        /* NOTE: Regular behaviour of instruction on original CHIP8 */
        chip8->v[x] = chip8->v[y];
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
    if (chip8->v[x] != chip8->v[y]) {
      chip8->pc += 2;
    }
    break;
  case 0xa: /* SET I */
    chip8->i = nnn;
    break;
  case 0xb: /* JUMP WITH OFFSET */
  {
    chip8->pc =
        chip8->config.jump_quirk ? nnn + chip8->v[x] : nnn + chip8->v[0];
    if (chip8->pc >= CHIP8_RAM_SIZE) {
      fprintf(stderr, "Out of memory error.");
      goto exit_routine;
    }
    break;
  }
  case 0xc: /* RANDOM */
    // TODO: implement emulator PRNG
    chip8->v[x] = (uint8_t)(rand() & nn);
    break;
  case 0xd: /* DRAW SCREEN */
  {
    draw_screen(chip8, instruction);
    break;
  }
  case 0xe:
    switch (nn) {
    case 0x9e: /* SKIP if PRESSED */
      if (chip8->keyboard[chip8->v[x] & 0x0f]) {
        chip8->pc += 2;
      }
      break;
    case 0xa1: /* SKIP if NOT PRESSED */
      if (!chip8->keyboard[chip8->v[x] & 0x0f]) {
        chip8->pc += 2;
      }
      break;
    default:
      fprintf(stderr, "CPU Error: Unknown instruction (instruction starting "
                      "with nibble 0xe).\n");
      goto exit_routine;
    }
    break;
  case 0xf:
    switch (nn) {

    case 0x07: /* SET Vx to TIMER */
      chip8->v[x] = chip8->dt;
      break;
    case 0x15: /* SET TIMER to Vx */
      chip8->dt = chip8->v[x];
      break;
    case 0x18: /* SET SOUND to Vx */
      chip8->st = chip8->v[x];
      break;
    case 0x1e: /* ADD to I */
      chip8->i += chip8->v[x];
      if (chip8->i >= 0x1000) {
        // NOTE: not truly COSMAC VIP property, but some machines rely on Vf to
        // be set if addition overflows.
        // None program relies on non happening of overflow.

        chip8->v[0xf] = 1;
      }
      break;
    case 0x0a: /* GET KEY */
      if (get_key(chip8, x)) {
        chip8->pc -= 2;
      }
      break;
    case 0x29: /* FONT */
    {
      uint8_t hex_char = chip8->v[x] & 0x0f;
      chip8->i = (uint16_t)chip8->config.font_start_addr +
                 hex_char * 5; /* NOTE: 5 is hardcoded height of fonts in
                                  bytes. Only works for original chip8. */
    } break;
    case 0x33: /* BCD */
    {
      uint8_t value = chip8->v[x];
      chip8->memory[chip8->i] = value / 100;
      chip8->memory[(chip8->i + 1) & 0x0fff] = (value / 10) % 10;
      chip8->memory[(chip8->i + 2) & 0x0fff] = value % 10;
    } break;
    case 0x55: /* STORE (ambiguous instruction) */
    {
      /* NOTE: insturction is ambiguous in terms of absolute or relative
       * positioning upon storing. For testing purposes only original COSMAC VIP
       * will behave as modern implemetation too. */

      uint16_t inx = chip8->i;
      for (uint8_t offset = 0; offset <= x; offset++) {
        inx = chip8->config.load_store_quirk ? chip8->i + offset : chip8->i++;
        chip8->memory[inx & 0x0fff] = chip8->v[offset];
      }
    } break;
    case 0x65: /* LOAD (ambiguous instruction) */
    {
      /* NOTE: Same remarks as for STORE instruction. */

      uint16_t inx = chip8->i;
      for (uint8_t offset = 0; offset <= x; offset++) {
        inx = chip8->config.load_store_quirk ? chip8->i + offset : chip8->i++;
        chip8->v[offset] = chip8->memory[inx & 0x0fff];
      }
    } break;
    default:
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
