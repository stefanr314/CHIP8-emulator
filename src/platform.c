#include "../include/chip8.h"
#include "../include/loader.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_stdinc.h>
#include <stdlib.h>
#include <time.h>

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
  // SDL_Init(SDL_VIDEO_DRIVER_DUMMY);

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

  double dt_cpu = 1.0 / chip8_config.simulation_hz;
  double dt_timer = 1.0 / chip8_config.timer_hz;

  double accu_cpu = 0.0;
  double accu_timer = 0.0;

  const double MAX_FRAME_TIME =
      10.0 * dt_cpu; /* Allow for maximum of 10 steps simulation advancement*/

  clock_t prev = clock();

  /* game loop */
  for (;;) {
    // record time, bound frame rate
    clock_t current = clock();
    double frame_time = ((double)current - prev) / CLOCKS_PER_SEC;
    if (frame_time > MAX_FRAME_TIME) {
      frame_time = MAX_FRAME_TIME;
    }
    prev = current;

    // update accu times
    accu_cpu += frame_time;
    accu_timer += frame_time;

    // update physical simulation logic
    while (accu_cpu >= dt_cpu) {
      chip8_cycle(&chip8);
      accu_cpu -= dt_cpu;
    }

    while (accu_timer >= dt_timer) {
      /* TODO: decrement dt i st timers */
      accu_timer -= dt_timer;
    }

    // update screen
    /* TODO: implement render logic */

    // use some interrupted wait instead of busy wait
  }
}
