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

typedef struct PlatformEvent {
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
 * Returns: the pointer to such context.
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
 * Returns: status of success: 0 if frame got (eventually) rendered to screen; 1
 * otherwise.
 * */
extern int platform_render_frame(Platform *platform, const uint8_t *framebuffer,
                                 uint32_t base_w, uint32_t base_h);

/*
 * Function for getting ticks/counter with high resolution timers.
 *
 * Returns: the value of counter as 64b number.
 * */
extern uint64_t platform_get_ticks(void);

/*
 * Function for getting the clock frequency at which the hr timer counts. To get
 * the value of elapsed time (in seconds) divide the counter value with the
 * frequency.
 *
 * Returns: the frequency at which hr timer works.
 * */
extern uint64_t platform_get_clock_frequency(Platform *platform);

/*
 * Function to delay the execution of process for some time. Time is represented
 * in ms.
 *
 * Args: time as 32b unsigned int, in ms.
 *
 * */
extern void platform_sleep_ms(uint32_t ms);

/*
 * Function for polling the events of pheripherals (such as keyboard, mouse, and
 * so on). For CHIP8 simulation purposes only keyboard presses are of
 * significant matter.
 *
 * Args: pointer to event that gets filled with data if significant events
 * occured.
 *
 * Returns: the state of event queue; 0 if there was no events to be
 * polled, 1 otherwise.
 * */
extern int platform_poll_event(PlatformEvent *event);

extern void platform_make_beep(Platform *platform, int beep_active);
#endif // !PLATFORM_H
