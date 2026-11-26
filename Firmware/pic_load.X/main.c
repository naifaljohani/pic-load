/**
 * @file    main.c
 * @brief   PIC Load firmware: start-up and task table.
 *
 * Board : PIC Load control board, PIC16F1779, XC8, internal osc 16 MHz
 * Pins  : see core/board.h
 *
 * Structure (tick + cooperative tasks)
 *
 *   Interrupts (core/isr.c) - short, on time, never wait
 *     IOC edge     encoder decoding
 *     1 ms tick    time base, encoder backup sample, keypad scan,
 *                  button debounce
 *
 *   Tasks (table below) - run from the main loop by core/scheduler.c
 *     Every task runs to completion and returns; none may wait or delay.
 *
 * | Task               | Period | Module          | Work                     |
 * |--------------------|--------|-----------------|--------------------------|
 * | encoder_task_100ms | 100 ms | drivers/encoder | speed window             |
 * | ui_task            |  20 ms | app/ui          | button, redraw, GLCD I2C |
 *
 * Folders
 *   app/      application: screens, user interface
 *   core/     board pins, clock, tick, scheduler, interrupt handler
 *   drivers/  hardware drivers: encoder, button, keypad, I2C, GLCD
 *   util/     plain C helpers without hardware access
 */
#include "system.h"
#include "tick.h"
#include "scheduler.h"
#include "i2c.h"
#include "st7920_glcd.h"
#include "encoder.h"
#include "button.h"
#include "keypad.h"
#include "ui.h"

/* ---------------- task table ---------------- */
static task_t tasks[] = {
    /* function            period ms */
    { encoder_task_100ms,  100, 0 },
    { ui_task,              20, 0 },
};
#define TASK_COUNT  ((uint8_t)(sizeof(tasks) / sizeof(tasks[0])))

void main(void)
{
    system_init();              // clock, LED, pull-ups off

    __delay_ms(100);            // let the GLCD and expander settle
    i2c_init();                 // SCL = RC3, SDA = RC4 (frees a stuck bus first)
    glcd_init();
    ui_init();                  // graphics mode, fixed screen parts

    encoder_init();
    button_init();
    keypad_init();
    tick_init();

    sched_init(tasks, TASK_COUNT);
    INTCONbits.GIE = 1;

    for (;;)
    {
        sched_run(tasks, TASK_COUNT);
    }
}
