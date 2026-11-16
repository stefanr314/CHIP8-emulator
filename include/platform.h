#ifndef PLATFORM_H
#define PLATFORM_H 1
#include <stdint.h>
/* Abstract Data Type definition */
typedef struct Platform Platform;

/* Data Type definitions */
typedef enum PlatformEventType {
  EMULATOR_QUIT_EVENT,
  KEY_PRESSED_EVENT,
  KEY_RELEASED_EVENT,
} PlatformEventType;

typedef struct PlatfromEvent {
  PlatformEventType event_type;
  union KeyIndex {
    uint8_t index;
  } key_index;
} PlatformEvent;

/* Platform API */
/* Function for creating the platform responsible for working with variety of
 * different drivers of subsystems e.g. graphics, audio, timers, keyboard, etc.
 *
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

/*
 * Function for render frame from framebuffer with base width and height set.
 *
 * Returns status of success: 0 if frame got (eventually) rendered to screen; 1
 * otherwise.
 * */
extern int platform_render_frame(Platform *platform, const uint8_t *framebuffer,
                                 uint32_t base_w, uint32_t base_h);

extern uint64_t platform_get_ticks(void);

extern uint64_t platform_get_clock_frequency(Platform *platform);

extern void platform_sleep_ms(uint32_t ms);

extern int platform_poll_event(PlatformEvent *event);
#endif // !PLATFORM_H
