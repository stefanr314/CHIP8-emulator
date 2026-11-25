#include "../include/platform.h"
#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_scancode.h>
#include <SDL_timer.h>
#include <SDL_video.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define SOUND_FREQ 440
#define AMPL_S16 2800

typedef void (*SoundGeneratorStrategy)(void *userdata, uint8_t *stream,
                                       int len);

typedef struct AudioData {
  bool beep_active;
  SoundGeneratorStrategy strategy;
  uint32_t sample_counter;
  int32_t ampl;
  uint32_t period_samples;
  uint32_t half_period_samples;
} AudioData;

struct Platform {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *screen_texture;
  uint64_t clock_frequency;
  SDL_Event event;
  SDL_AudioDeviceID audio_dev;
  AudioData audio_data;
  SDL_AudioSpec obtained;
};

static inline int scancode_to_hex_keyboard_index(SDL_Scancode scancode) {
  if (scancode == SDL_SCANCODE_1) {
    return 0x01;
  } else if (scancode == SDL_SCANCODE_2) {
    return 0x02;
  } else if (scancode == SDL_SCANCODE_3) {
    return 0x03;
  } else if (scancode == SDL_SCANCODE_4) {
    return 0x0c;
  } else if (scancode == SDL_SCANCODE_Q) {
    return 0x04;
  } else if (scancode == SDL_SCANCODE_W) {
    return 0x05;
  } else if (scancode == SDL_SCANCODE_E) {
    return 0x06;
  } else if (scancode == SDL_SCANCODE_R) {
    return 0x0d;
  } else if (scancode == SDL_SCANCODE_A) {
    return 0x07;
  } else if (scancode == SDL_SCANCODE_S) {
    return 0x08;
  } else if (scancode == SDL_SCANCODE_D) {
    return 0x09;
  } else if (scancode == SDL_SCANCODE_F) {
    return 0x0e;
  } else if (scancode == SDL_SCANCODE_Z) {
    return 0x0a;
  } else if (scancode == SDL_SCANCODE_X) {
    return 0x00;
  } else if (scancode == SDL_SCANCODE_C) {
    return 0x0b;
  } else if (scancode == SDL_SCANCODE_V) {
    return 0x0f;
  }

  return -1;
}

static inline void generate_sound_freq(AudioData *data, uint32_t sound_freq,
                                       uint32_t sample_rate,
                                       int32_t amplitude) {
  data->ampl = amplitude;
  data->period_samples = sample_rate / sound_freq;
  data->half_period_samples = data->period_samples / 2;
}

static void audio_dispatch_callback(void *userdata, uint8_t *stream, int len);
static void generate_s16(void *userdata, uint8_t *stream, int len);
static void generate_u8(void *userdata, uint8_t *stream, int len);
static void generate_f32(void *userdata, uint8_t *stream, int len);

Platform *platform_create(const char *window_title, uint32_t base_width,
                          uint32_t base_height, uint32_t scale) {

  Platform *platform = calloc(1, sizeof(struct Platform));
  if (!platform) {
    fprintf(stderr, "Error when working with memory allocation system.\n");
    return NULL;
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,
              "0"); /* "0" == nearest pixel sampling*/

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    fprintf(stderr,
            "Library error upon loading the subsystems. SDL Error: %s\n",
            SDL_GetError());
    free(platform);
    return NULL;
  }

  platform->window = SDL_CreateWindow(
      window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      (int)(scale * base_width), (int)(scale * base_height),
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!platform->window) {
    fprintf(stderr,
            "Library error when loading the video subsystem. SDL Error: %s\n",
            SDL_GetError());
    goto error_routine;
  }

  platform->renderer =
      SDL_CreateRenderer(platform->window, -1, SDL_RENDERER_ACCELERATED);
  if (!platform->renderer) {
    fprintf(stderr,
            "Library error upon loading the video subsystem. SDL Error: %s\n",
            SDL_GetError());
    goto error_routine;
  }

  SDL_RenderSetLogicalSize(platform->renderer, (int)base_width,
                           (int)base_height);
  SDL_SetRenderDrawColor(platform->renderer, 0x00, 0x00, 0x00,
                         SDL_ALPHA_OPAQUE);

  platform->screen_texture = SDL_CreateTexture(
      platform->renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
      (int)base_width, (int)base_height);
  if (!platform->screen_texture) {
    fprintf(stderr, "Library error on video subsystem. SDL Error: %s\n",
            SDL_GetError());
    goto error_routine;
  }

  platform->clock_frequency = SDL_GetPerformanceFrequency();

  const SDL_AudioSpec desired_spec = {.freq = 44100,
                                      .format = AUDIO_S16SYS,
                                      .channels = 1,
                                      .silence = 0,
                                      .samples = 512,
                                      .size = (512 * 1 * 2),
                                      .callback = audio_dispatch_callback,
                                      .userdata = &platform->audio_data};
  platform->audio_dev =
      SDL_OpenAudioDevice(NULL, 0, &desired_spec, &platform->obtained, 0);
  if (!platform->audio_dev) {
    fprintf(stderr, "Error upon loading the audio subsystem. Error: %s\n",
            SDL_GetError());
    goto error_routine;
  }

  switch (platform->obtained.format) {

  case AUDIO_S16SYS:
    generate_sound_freq(&platform->audio_data, SOUND_FREQ,
                        (uint32_t)platform->obtained.freq, AMPL_S16);
    platform->audio_data.beep_active = false;
    platform->audio_data.strategy = generate_s16;
    platform->audio_data.sample_counter = 0;

    break;
  default:
    fprintf(stderr, "Error upon loading the audio subsystem. Audio format not "
                    "recognized.\n");
    goto error_routine;
  }

  SDL_PauseAudioDevice(platform->audio_dev, 0);
  return platform;
error_routine:
  platform_destroy(platform);
  return NULL;
}

/*
 * Wrapper function used for delegating to concrete implementation of strategy
 */
static void audio_dispatch_callback(void *userdata, uint8_t *stream, int len) {
  ((AudioData *)userdata)->strategy(userdata, stream, len);
}

static void generate_s16(void *userdata, uint8_t *stream, int len) {
  AudioData *audio_data = (AudioData *)userdata;
  if (!audio_data->beep_active) {
    audio_data->sample_counter = 0;
    // make silence - 0 is silence
    memset((void *)stream, 0, (size_t)len);
    return;
  }
  uint32_t sample_counter = audio_data->sample_counter;
  int16_t *write_at = (int16_t *)stream;
  int16_t ampl_h = (int16_t)audio_data->ampl;
  int16_t ampl_l = (int16_t)(-audio_data->ampl); // NOTE: check me...
  uint32_t buffer_samples = (uint32_t)((size_t)len / sizeof(int16_t));

  for (uint32_t sample = 0; sample < buffer_samples; sample++) {
    if (sample_counter < audio_data->half_period_samples) {
      *write_at++ = ampl_h;
    } else {
      *write_at++ = ampl_l;
    }

    sample_counter = (sample_counter + 1) % audio_data->period_samples;
  }

  audio_data->sample_counter = sample_counter;
}

int platform_render_frame(Platform *platform, const uint8_t *framebuffer,
                          uint32_t base_w, uint32_t base_h) {
  // acquire pointer to load the texture buffer
  int status = EXIT_SUCCESS;
  void *raw_texture = NULL;
  int pitch = 0;

  // lock the texture
  int not_locked =
      SDL_LockTexture(platform->screen_texture, NULL, &raw_texture, &pitch);
  if (not_locked) {
    fprintf(stderr, "Error when working with video subsystem. SDL Error: %s\n",
            SDL_GetError());
    return EXIT_FAILURE;
  }

  uint8_t *texture_framebuffer = (uint8_t *)raw_texture;
  const uint8_t *src_buffer = framebuffer;
  uint8_t *dst_buffer = texture_framebuffer;
  uint32_t *dst_pixels = (uint32_t *)dst_buffer;

  for (uint32_t y = 0; y < base_h; y++) {
    src_buffer = framebuffer + (y * base_w);
    dst_buffer = texture_framebuffer + (y * (uint32_t)pitch);
    dst_pixels = (uint32_t *)dst_buffer;

    for (uint32_t x = 0; x < base_w; x++) {
      dst_pixels[x] = src_buffer[x] ? 0xffffffff : 0x00000000;
    }
  }
  SDL_UnlockTexture(platform->screen_texture);

  // do the clear, draw, present chain
  (void)SDL_SetRenderDrawColor(platform->renderer, 0x00, 0x00, 0x00,
                               SDL_ALPHA_OPAQUE);
  (void)SDL_RenderClear(platform->renderer);
  (void)SDL_RenderCopy(platform->renderer, platform->screen_texture, NULL,
                       NULL);
  SDL_RenderPresent(platform->renderer);

  return status;
}

void platform_make_beep(Platform *platform, int beep_active) {

  SDL_LockAudioDevice(platform->audio_dev);
  platform->audio_data.beep_active = (bool)beep_active;
  SDL_UnlockAudioDevice(platform->audio_dev);
}

uint64_t platform_get_ticks(void) { return SDL_GetPerformanceCounter(); }

uint64_t platform_get_clock_frequency(Platform *platform) {
  return platform->clock_frequency;
}

void platform_sleep_ms(uint32_t ms) { SDL_Delay(ms); }

int platform_poll_event(PlatformEvent *event) {
  if (!SDL_PollEvent(NULL))
    return 0;

  SDL_Event sdl_event;
  SDL_Scancode scancode;
  int inx = -1;
  while (SDL_PollEvent(&sdl_event)) {
    switch (sdl_event.type) {
    case SDL_QUIT:
      event->event_type = EMULATOR_QUIT_EVENT;
      return 1;
    case SDL_KEYDOWN:
      scancode = sdl_event.key.keysym.scancode;
      if (!sdl_event.key.repeat &&
          (inx = scancode_to_hex_keyboard_index(scancode)) != -1) {

        event->event_type = KEY_PRESSED_EVENT;
        event->key_index.index = (uint8_t)inx;
        return 1;
      }
      break;
    case SDL_KEYUP:
      scancode = sdl_event.key.keysym.scancode;
      if ((inx = scancode_to_hex_keyboard_index(scancode)) != -1) {

        event->event_type = KEY_RELEASED_EVENT;
        event->key_index.index = (uint8_t)inx;
        return 1;
      }
      break;
    }
  }

  return 0;
}

// cleaning routine; no-op if some fields are not set
void platform_destroy(Platform *platform) {
  if (!platform)
    return;

  if (platform->audio_dev) {
    SDL_CloseAudioDevice(platform->audio_dev);
  }
  if (platform->screen_texture) {
    SDL_DestroyTexture(platform->screen_texture);
  }
  if (platform->renderer) {
    SDL_DestroyRenderer(platform->renderer);
  }
  if (platform->window) {
    SDL_DestroyWindow(platform->window);
  }
  SDL_Quit();
  free(platform);
  platform = NULL;
}
