#include "../include/chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *init_chip8(Chip8 *chip8, const Chip8Config *chip8_config) {
  chip8->config = *chip8_config;

  /* set array explicitly */
  memset(&chip8->memory, 0,
         chip8_config->ram_size); /* set just the valid fields */
  memset(&chip8->v, 0, chip8_config->gpr_count);
  memset(&chip8->stack, 0, chip8_config->stack_depth);
  memset(&chip8->screen, 0,
         (chip8_config->screen_height * chip8_config->screen_width));

  chip8->pc = chip8_config->entry_point_addr;
  chip8->sp = 0;
  chip8->dt = 0;
  chip8->st = 0;

  return chip8;
}

int chip8_cycle(Chip8 *chip8) {
  uint16_t instruction;
  uint8_t first_nibble;
  uint8_t last_byte;
  uint8_t last_nibble;

  /* FIXME: return status of cycle... */

  // fetch phase
  if (chip8->pc >= CHIP8_RAM_SIZE - 1) {
    fprintf(stderr, "CPU Trap: Memory overflow. PC out of bounds: (0x%.4x)\n",
            chip8->pc);
    goto EXIT_ROUTINE;
  }
  instruction =
      (chip8->memory[chip8->pc] << 8) | (chip8->memory[chip8->pc + 1]);
  chip8->pc += 2;
  first_nibble = (0xf000 & instruction) >> 12;

  printf("%.1x\n", first_nibble);

  // decode phase
  switch (first_nibble) {
  case 0:
    if (instruction == 0x00e0) {
      (void)printf("CLEAR SCREEN\n");
    } else if (instruction == 0x00ee) {
      (void)puts("RETURN (from subroutine)");

      // pop operation
      // check STACK UNDERFLOW
      chip8->pc = chip8->stack[chip8->sp];
      chip8->sp--;
    } else {
      (void)fprintf(
          stderr, "CPU Error: Insturction for writing direct machine code not "
                  "allowed.");
      // exit the loop
      goto EXIT_ROUTINE;
    }
    break;
  case 1:
    printf("JUMP: %.3x", instruction & 0x0fff);

    chip8->pc = instruction & 0x0fff;
    break;
  case 2:
    printf("CALL: %.3x\n", instruction & 0x0fff);
    // prior to setting pc to new address save it's content to stack

    // implent push operation.
    // check STACK OVERFLOW
    chip8->stack[chip8->sp] = chip8->pc;
    chip8->sp++;

    chip8->pc = instruction & 0x0fff;
    break;
  case 3:
    // at register uint8_t reg_index = (0x0f00 & instruction) >> 8
    // value to be compared uint8_t const_value = 0x00ff & instruction or just
    // (uint8_t)instruction
    puts("SKIP CONDITIONALlY IF Vx EQUAL TO CONST");
    break;
  case 4:
    puts("SKIP CONDITIONALlY IF Vx NOT EQUAL TO CONST");
    break;
  case 5:
    puts("SKIP CONDITIONALlY IF Vx EQUAL TO Vy");
    break;
  case 6:
    puts("SET Vx TO CONST");
    break;
  case 7:
    puts("ADD CONST TO Vx (not overflow)");
    break;
  case 8:
    last_nibble = instruction & 0x000f;
    switch (last_nibble) {
    case 0:
      puts("SET VX TO Vy");
      break;
    case 1:
      puts("BINARY OR");
      break;
    case 2:
      puts("BINARY AND");
      break;
    case 3:
      puts("LOGICAL XOR");
      break;
    case 4:
      puts("ADD (with overflow)");
      break;
    case 5:
      puts("SUBTRACT");
      break;
    case 6:
      puts("SHIFT RIGHT 1b(!!!ambiguous instsruction!!!)");
      break;
    case 7:
      puts("SUBTRACT");
      break;
    case 0xe:
      puts("SHIFT LEFT 1b(!!!ambiguous instsruction!!!)");
      break;
    default:
      fprintf(stderr, "CPU Error: Unknown instruction (instruction starting "
                      "with nibble 0x8).\n");
      goto EXIT_ROUTINE;
    }
    break;
  case 9:
    puts("SKIP CONDITIONALlY IF Vx NOT EQUAL TO Vy");
    break;
  case 0xa:
    puts("SET Immediate REG");
    break;
  case 0xb:
    puts("JUMP WITH OFFSET (!!!ambiguous instruction!!!)");
    break;
  case 0xc:
    puts("RANDOM");
    break;
  case 0xd:
    puts("DISPLAY");
    break;
  case 0xe:
    last_byte = (uint8_t)instruction;
    if (last_byte == 0x9e) {
      puts("SKIP INSTRUCTION IF KEY PRESSED");
    } else if (last_byte == 0xa1) {
      puts("SKIP INSTRUCTION IF KEY NOT PRESSED");
    } else {
      fprintf(stderr, "CPU Error: Unknown instruction (instruction starting "
                      "with nibble 0xe).\n");
      goto EXIT_ROUTINE;
    }
    break;
  case 0xf:
    last_byte = (uint8_t)instruction;
    if (last_byte == 0x07) {
      puts("SET Vx TO TIMER VALUE");
    } else if (last_byte == 0x15) {
      puts("SET DELAY TIMER");
    } else if (last_byte == 0x18) {
      puts("SET SOUND TIMER");
    } else if (last_byte == 0x1e) {
      puts("ADD TO Immediate REG");
    } else if (last_byte == 0x0a) {
      puts("GET KEY (blocking)");
      // decrement pc -= 2 to simulate blocking -> polling
    } else if (last_byte == 0x29) {
      puts("FONT");
    } else if (last_byte == 0x33) {
      puts("BINARY CODED DECIMAL CONVERSION");
    } else if (last_byte == 0x55) {
      puts("STORE MEMORY");
    } else if (last_byte == 0x65) {
      puts("LOAD MEMORY");
    } else {
      fprintf(stderr, "CPU Error: Unknown instruction (instruction starting "
                      "with nibble 0xf).\n");
      goto EXIT_ROUTINE;
    }
    break;
  default:
    fprintf(stderr, "CPU Error: Unknown instruction.\n");
    goto EXIT_ROUTINE;
  }

EXIT_ROUTINE:;

  return EXIT_SUCCESS;
}
