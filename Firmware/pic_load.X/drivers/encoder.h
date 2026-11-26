/**
 * @file    encoder.h
 * @brief   Rotary encoder (KY-040 style) on CLK = RA0, DT = RA1.
 *
 * Decoding
 *   - Interrupt-on-change on both pins, both edges: every quadrature edge
 *     is caught in hardware, even while the main loop is busy with I2C.
 *   - A backup sample every 1 ms (tick) catches anything the edge
 *     interrupt missed.
 *   - One count per detent (rest position = both pins high). Tolerant of a
 *     single missed edge; impossible transitions are counted as "miss".
 *   - The status LED (board.h) toggles on each detent.
 *
 * Speed
 *   Detents per second over a 500 ms window (5 slots of 100 ms), updated
 *   by encoder_task_100ms().
 *
 * Context: getters and encoder_reset() are for main/tasks only.
 */
#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

#define ENCODER_DETENTS_PER_REV 20  /**< clicks per full turn (KY-040: 20) */

/** Pins as inputs with pull-ups and Schmitt levels, enable IOC.
 *  Call after system_init(); waits 5 ms for the pull-ups to settle. */
void encoder_init(void);

/** Position in detents since start or the last encoder_reset(). */
int16_t encoder_get_pos(void);

/** Speed in detents/s, signed (+ = clockwise). */
int16_t encoder_get_speed(void);

/** Number of invalid transitions (missed edges). Should stay 0. */
uint16_t encoder_get_miss(void);

/** Set position and miss counter back to 0. */
void encoder_reset(void);

/** Task, every 100 ms: updates the speed window. */
void encoder_task_100ms(void);

/* ---- ISR hooks: call only from isr.c ---- */

/** IOC interrupt: clears the encoder IOC flags and decodes. */
void encoder_isr_edge(void);

/** 1 ms tick: backup sample of the pins. */
void encoder_isr_tick(void);

#endif /* ENCODER_H */
