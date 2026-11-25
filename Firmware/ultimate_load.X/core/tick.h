/**
 * @file    tick.h
 * @brief   1 ms system tick (Timer2) and millisecond time base.
 *
 * Timer2: Fosc/4 = 4 MHz, prescaler 1:16 -> 250 kHz, period 250 -> 1 ms.
 * The counter is 16 bits and wraps every 65.536 s. Compare times with
 * subtraction, e.g. (uint16_t)(tick_ms() - start) >= 100, which stays
 * correct across the wrap.
 */
#ifndef TICK_H
#define TICK_H

#include <stdint.h>

/** Start Timer2 and enable its interrupt (GIE must be set by the caller). */
void tick_init(void);

/** Milliseconds since tick_init(), wraps at 65535. Safe to call from main. */
uint16_t tick_ms(void);

/* ---- ISR hook: call only from isr.c ---- */

/** 1 ms timer interrupt: clears the flag and advances the counter. */
void tick_isr(void);

#endif /* TICK_H */
