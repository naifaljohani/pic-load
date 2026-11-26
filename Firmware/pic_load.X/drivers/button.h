/**
 * @file    button.h
 * @brief   Encoder push button on RA2 (pressed = low), debounced.
 *
 * Sampled every 1 ms; the state changes after 8 equal samples in a row
 * (8 ms). Each new press sets an event that main takes with
 * button_take_press().
 *
 * Context: getters are for main/tasks only.
 */
#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

/** Pin as input with pull-up and Schmitt level. Call after system_init(). */
void button_init(void);

/** 1 while the button is held (debounced). */
uint8_t button_is_down(void);

/** 1 once for each new press, then 0 until the next press. */
uint8_t button_take_press(void);

/* ---- ISR hook: call only from isr.c ---- */

/** 1 ms tick: debounce. */
void button_isr_tick(void);

#endif /* BUTTON_H */
