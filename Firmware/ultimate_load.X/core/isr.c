/**
 * @file    isr.c
 * @brief   The single interrupt handler of the PIC16F1779.
 *
 * The PIC16 has one interrupt vector, so this function checks each flag
 * and calls the hook of the module that owns it. Hooks are short and never
 * wait. Nothing else in the firmware should define an interrupt function.
 *
 * | Source                | Rate        | Hooks called                         |
 * |-----------------------|-------------|--------------------------------------|
 * | IOC on RA0/RA1        | every edge  | encoder_isr_edge()                   |
 * | Timer2                | 1 ms        | tick_isr(), encoder_isr_tick(),      |
 * |                       |             | button_isr_tick(), keypad_isr_tick() |
 */
#include "system.h"
#include "tick.h"
#include "encoder.h"
#include "button.h"
#include "keypad.h"

void interrupt isr(void)
{
    /* ---- encoder edge: IOC on RA0 (CLK) / RA1 (DT), both edges ---- */
    if (INTCONbits.IOCIF)
    {
        encoder_isr_edge();
    }

    /* ---- 1 ms tick (Timer2) ---- */
    if (PIR1bits.TMR2IF)
    {
        tick_isr();                 // clears the flag
        encoder_isr_tick();         // backup sample (catches anything missed)
        keypad_isr_tick();          // one keypad row per tick
        button_isr_tick();          // debounce
    }
}
