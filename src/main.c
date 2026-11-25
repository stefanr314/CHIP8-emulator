#include "../include/chip8.h"
#include "../include/loader.h"
#include "../include/platform.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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

  Chip8 chip8;
  // load the config for chip8
  Chip8Config chip8_config = CONFIG_PRESETS[0]; /* NOTE: hardcode it for now */

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

  /* Start up the platform */
  Platform *platform =
      platform_create("CHIP8 - Emulator", chip8_config.screen_width,
                      chip8_config.screen_height, 15);

  double dt_cpu = 1.0 / chip8_config.simulation_hz;
  double dt_timer = 1.0 / chip8_config.timer_hz;
  double accu_cpu = 0.0;
  double accu_timer = 0.0;

  const double MIN_FRAME_TIME =
      1.0 /
      chip8_config
          .display_refresh_rate; /* since VSYNC does not effect emulated guest
                                    screen bound this frame time too*/
  const double MAX_FRAME_TIME =
      5.0 * MIN_FRAME_TIME; /* Cap frame time to prevent spiral of death. */

  uint64_t prev = platform_get_ticks();
  uint64_t clock_frequency = platform_get_clock_frequency(platform);

  bool quit = false;

  PlatformEvent event = {.event_type = 0,
                         .key_index.index = 0}; /* make memory checkers happy */
  /* game loop */
  while (!quit) {

    /* check events */
    while (platform_poll_event(&event)) {
      if (event.event_type == EMULATOR_QUIT_EVENT) {
        quit = true;
      } else if (event.event_type == KEY_PRESSED_EVENT) {
        chip8.keyboard[event.key_index.index] = 1;
      } else if (event.event_type == KEY_RELEASED_EVENT) {
        chip8.keyboard[event.key_index.index] = 0;
      }
    }

    uint64_t current = platform_get_ticks();
    double frame_time = (double)(current - prev) / clock_frequency;
    if (frame_time > MAX_FRAME_TIME) {
      frame_time = MAX_FRAME_TIME;
    }
    prev = current;

    // update accu times
    accu_cpu += frame_time;
    accu_timer += frame_time;

    // update physical simulation logic
    while (accu_cpu >= dt_cpu) {
      int not_cycled = chip8_cycle(&chip8);
      if (not_cycled) {
        fprintf(stderr, "Emulated CPU suffered internal error.\n");
        goto exit_routine;
      }
      accu_cpu -= dt_cpu;
    }

    while (accu_timer >= dt_timer) {
      // NOTE: since dt and st are unsigned this is false only when both are
      // unset

      if (chip8.dt > 0) {
        chip8.dt--;
      }
      if (chip8.st > 0) {
        chip8.st--;
      }
      accu_timer -= dt_timer;
    }

    // FIXME: is this the way...
    platform_make_beep(platform, chip8.st > 0);

    if (chip8.draw_flag) {
      int not_rendered = platform_render_frame(platform, chip8.screen,
                                               chip8_config.screen_width,
                                               chip8_config.screen_height);
      if (not_rendered) {
        fprintf(stderr, "Renderer suffered critical error terminating...");
        goto exit_routine;
      }
      chip8.draw_flag = 0;
    }

    /* Sleep the rest of time frame. */
    uint64_t elapsed_ticks = platform_get_ticks() - current;
    double elapsed_sec = (double)elapsed_ticks / clock_frequency;
    if (elapsed_sec < MIN_FRAME_TIME) {
      double gap_ms = (MIN_FRAME_TIME - elapsed_sec) * 1000.0;
      platform_sleep_ms((uint32_t)gap_ms); /* gap drift caught by accumulators*/
    }
  }

exit_routine:
  platform_destroy(platform);

  return EXIT_SUCCESS;
}
