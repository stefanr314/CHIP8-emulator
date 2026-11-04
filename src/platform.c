#include "../include/chip8.h"
#include "../include/loader.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    (void)fprintf(stderr,
                  "Please provide the name of the rom file to be loaded. "
                  "The format is as follows: <exec_file> <rom_filename>.\n");
    return EXIT_FAILURE;
  }

  if (argc != 2) {
    (void)fprintf(stderr,
                  "Please provide only single rom file to be loaded.\n");
    return EXIT_FAILURE;
  }

  char *filename = argv[1];

  Chip8 chip8;
  // load the config for chip8
  Chip8Config chip8_config = CONFIG_PRESETS[0]; /* hardcode it for now */

  // init the chip8 to proper configuration
  (void)init_chip8(&chip8, &chip8_config);

  // load the rom in the memory
  int notLoaded =
      load_rom_file(filename, &chip8.memory[chip8_config.entry_point_addr],
                    NULL, chip8_config.max_rom_size);
  if (notLoaded) {
    return EXIT_FAILURE;
  }

  printf("\nLets play some games, shall we...\n");

  /* TODO: set the timers and the loop. */
}
