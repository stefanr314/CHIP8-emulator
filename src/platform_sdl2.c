#include "../include/platform.h"
#include <SDL.h>
#include <SDL_error.h>
#include <SDL_hints.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_video.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

struct Platform {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *screen_texture;
};

Platform *platform_create(const char *window_title, uint32_t base_width,
                          uint32_t base_height, uint32_t scale) {

  Platform *platform = calloc(1, sizeof(struct Platform));
  if (!platform) {
    fprintf(stderr, "Error when working with memory allocation system.\n");
    return NULL;
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,
              "0"); /* "0" == nearest pixel sampling*/

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr,
            "Library error upon loading the subsystems. SDL Error: %s\n",
            SDL_GetError());
    free(platform);
    return NULL;
  }

  platform->window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                                      scale * base_width, scale * base_height,
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

  SDL_RenderSetLogicalSize(platform->renderer, base_width, base_height);
  SDL_SetRenderDrawColor(platform->renderer, 0x00, 0x00, 0x00,
                         SDL_ALPHA_OPAQUE);

  platform->screen_texture =
      SDL_CreateTexture(platform->renderer, SDL_PIXELFORMAT_RGBA32,
                        SDL_TEXTUREACCESS_STREAMING, base_width, base_height);
  if (!platform->screen_texture) {
    fprintf(stderr, "Library error on video subsystem. SDL Error: %s\n",
            SDL_GetError());
    goto error_routine;
  }

  return platform;
error_routine:
  platform_destroy(platform);
  return NULL;
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
    dst_buffer = texture_framebuffer + (y * pitch);
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

void platform_destroy(Platform *platform) {
  if (!platform)
    return;

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
