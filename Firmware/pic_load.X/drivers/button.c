/**
 * @file    button.c
 * @brief   Debounced push button.
 */
#include "system.h"
#include "board.h"
#include "button.h"

static volatile uint8_t btn_down;   // 1 while held (debounced)
static volatile uint8_t btn_event;  // set on each new press

void button_init(void)
{
    BTN_ANSEL &= (uint8_t)~BTN_MASK;    // digital
    BTN_TRIS  |= BTN_MASK;              // input
    BTN_INLVL |= BTN_MASK;              // Schmitt-trigger input level
    BTN_WPU   |= BTN_MASK;              // weak pull-up
}

uint8_t button_is_down(void)
{
    return btn_down;
}

uint8_t button_take_press(void)
{
    uint8_t e;
    ATOMIC_BEGIN();                     // read and clear in one step
    e = btn_event;
    btn_event = 0;
    ATOMIC_END();
    return e;
}

void button_isr_tick(void)
{
    static uint8_t hist = 0;

    // 8 equal samples in a row = debounced
    hist = (uint8_t)(hist << 1) | (BTN_IS_PRESSED() ? 1u : 0u);
    if (hist == 0xFFu && !btn_down) { btn_down = 1; btn_event = 1; }
    else if (hist == 0x00u)         { btn_down = 0; }
}
