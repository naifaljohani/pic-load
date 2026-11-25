/**
 * @file    tick.c
 * @brief   1 ms system tick (Timer2).
 */
#include "system.h"
#include "tick.h"

static volatile uint16_t tick_count;    // ms, written by the ISR only

void tick_init(void)
{
    // Timer2: Fosc/4 = 4 MHz, 1:16 -> 250 kHz, period 250 -> 1 ms
    T2CLKCON = 0x01;                // clock = Fosc/4
    T2HLT    = 0x00;                // free running, software gate
    T2PR     = 249;
    TMR2     = 0;
    T2CON    = 0xC0;                // ON = 1, CKPS = 1:16, OUTPS = 1:1
    PIR1bits.TMR2IF = 0;
    PIE1bits.TMR2IE = 1;
    INTCONbits.PEIE = 1;
}

uint16_t tick_ms(void)
{
    uint16_t t;
    ATOMIC_BEGIN();                 // 16-bit read, the ISR may change it
    t = tick_count;
    ATOMIC_END();
    return t;
}

void tick_isr(void)
{
    PIR1bits.TMR2IF = 0;
    tick_count++;
}
