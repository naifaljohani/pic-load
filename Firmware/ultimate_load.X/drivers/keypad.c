/**
 * @file    keypad.c
 * @brief   4x4 matrix keypad driver.
 */
#include "system.h"
#include "board.h"
#include "keypad.h"

#define KP_DEBOUNCE  5                  // equal full scans (5 x 4 ms = 20 ms)

// key at row r, column c = kp_map[r * 4 + c]
static const char kp_map[] = "123A456B789C*0#D";

/* ---- shared with the interrupt ---- */
static volatile char     kp_key;        // key held now (debounced), 0 = none
static volatile char     kp_last;       // last key pressed, 0 = none yet
static volatile uint16_t kp_count;      // number of presses

/* ---- interrupt only ---- */
static uint8_t kp_row;                  // row being driven low (0..3)

static void kp_select_row(uint8_t r)    // selected row = output low, rest = inputs
{
    KP_TRIS = (uint8_t)~(1u << r);
}

void keypad_init(void)
{
    KP_ANSEL  = 0x00;                   // port D digital
    KP_LAT    = 0x00;                   // selected row drives low
    KP_INLVL |= KP_COL_MASK;            // Schmitt-trigger levels on the columns
    KP_WPU   |= KP_COL_MASK;            // weak pull-ups on the columns
    kp_row    = 0;
    KP_TRIS   = 0xFE;                   // row 1 (RD0) selected, rest inputs
}

char keypad_get_held(void)
{
    return kp_key;
}

char keypad_get_last(void)
{
    return kp_last;
}

uint16_t keypad_get_count(void)
{
    uint16_t v;
    ATOMIC_BEGIN();
    v = kp_count;
    ATOMIC_END();
    return v;
}

// Called every 1 ms. The columns are read for the row that was selected on
// the previous tick (1 ms to settle), then the next row is selected.
void keypad_isr_tick(void)
{
    static char    found = 0;           // key seen during the current scan
    static char    cand  = 0;           // debounce candidate
    static uint8_t cnt   = 0;

    uint8_t cols = (uint8_t)(((uint8_t)~KP_PORT) >> 4);    // pressed = 1, bit0 = RD4
    if (cols && !found)
    {
        uint8_t c = 0;
        while (!(cols & 1u)) { cols >>= 1; c++; }
        found = kp_map[(uint8_t)(kp_row * 4u + c)];
    }

    if (++kp_row >= 4u)                 // end of a full scan
    {
        kp_row = 0;
        if (found != cand) { cand = found; cnt = 0; }
        else if (cnt < KP_DEBOUNCE && ++cnt == KP_DEBOUNCE && cand != kp_key)
        {
            kp_key = cand;
            if (cand) { kp_last = cand; kp_count++; }
        }
        found = 0;
    }
    kp_select_row(kp_row);
}
