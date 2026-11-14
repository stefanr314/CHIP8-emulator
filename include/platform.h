#ifndef PLATFORM_H
#define PLATFORM_H 1
#include <stdint.h>
/* Abstract Data Type definition */
typedef struct Platform Platform;

/* Platform API */
/* Function for creeating the platform responsible for working with variety of
 * different drivers of subsystems e.g. graphics, audio, timers, keyboard, etc.
 * Params: window title for video subsystem, base width and height of screen to
 * be emulated and scaling point for output screen.
 *
 * Returns the pointer to such context.
 */
extern Platform *platform_create(const char *window_title, uint32_t base_width,
                                 uint32_t base_height, uint32_t scale);

/* Function for destroying all the reesources taken away for platform
 * representation.
 * Param: pointer to platform to be destroyed. Function should
 * behave as no-op when platform not created.*/
extern void platform_destroy(Platform *platform);

extern int platform_render_frame(Platform *platform,
                                  const uint8_t *framebuffer, uint32_t base_w,
                                  uint32_t base_h);
#endif // !PLATFORM_H
