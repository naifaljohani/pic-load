/**
 * @file    system.h
 * @brief   Clock, basic board setup and interrupt helpers.
 *
 * Clock: internal oscillator, 16 MHz (Fosc/4 = 4 MHz instruction clock).
 */
#ifndef SYSTEM_H
#define SYSTEM_H

#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ 16000000UL   /**< internal oscillator, 16 MHz (used by __delay_ms) */

/**
 * @brief Run a block with interrupts off, then restore the previous state.
 *
 * Used to read or change multi-byte data shared with the interrupt:
 * @code
 *     ATOMIC_BEGIN();
 *     copy = shared_value;
 *     ATOMIC_END();
 * @endcode
 * Restores GIE as it was, so it is safe before interrupts are enabled.
 */
#define ATOMIC_BEGIN()  do { uint8_t gie_saved_ = INTCONbits.GIE; INTCONbits.GIE = 0
#define ATOMIC_END()    INTCONbits.GIE = gie_saved_; } while (0)

/**
 * @brief Oscillator at 16 MHz, status LED pin, all weak pull-ups off
 *        (each driver then enables only its own) and the pull-up master
 *        enable on. Call first in main().
 */
void system_init(void);

#endif /* SYSTEM_H */
