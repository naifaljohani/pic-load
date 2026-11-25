/**
 * @file    keypad.h
 * @brief   4x4 matrix keypad on port D.
 *
 * Wiring (see board.h): the 8-pin keypad connector is fitted in reverse pin
 * order, so the drive lines RD0..RD3 go to keypad columns 4..1 and the
 * sense lines RD4..RD7 go to keypad rows 4..1. The key map in keypad.c
 * accounts for this. In the text below, "row" means a drive line RD0..RD3
 * and "column" a sense line RD4..RD7.
 *
 * Layout as seen on the keypad (row 1 at the top, column 1 on the left):
 * @verbatim
 *     1 2 3 A
 *     4 5 6 B
 *     7 8 9 C
 *     * 0 # D
 * @endverbatim
 *
 * Scanning
 *   - One row per 1 ms tick: the selected row is an output driven low, the
 *     other rows are inputs, so two pressed keys can never short two
 *     outputs. Columns are inputs with pull-ups; a pressed key reads low.
 *   - The columns are read 1 ms after their row was selected (settling).
 *   - A full scan takes 4 ms; a key changes after 5 equal scans (20 ms).
 *   - If several keys are down, the first one in scan order is reported.
 *
 * Context: getters are for main/tasks only.
 */
#ifndef KEYPAD_H
#define KEYPAD_H

#include <stdint.h>

/** Port D digital, column pull-ups, row 1 selected.
 *  Call after system_init() and before the tick starts. */
void keypad_init(void);

/** Key held now as its character ('0'..'9', 'A'..'D', '*', '#'), 0 = none. */
char keypad_get_held(void);

/** Last key pressed (character), 0 = no key pressed yet. */
char keypad_get_last(void);

/** Number of key presses since start (wraps at 65535). */
uint16_t keypad_get_count(void);

/* ---- ISR hook: call only from isr.c ---- */

/** 1 ms tick: read one row, select the next. */
void keypad_isr_tick(void);

#endif /* KEYPAD_H */
