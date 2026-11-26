/**
 * @file    encoder.c
 * @brief   Rotary encoder driver.
 */
#include "system.h"
#include "board.h"
#include "encoder.h"

#define ENCODER_REVERSE 0           // 1 = swap the counting direction

/* ---- shared with the interrupt ---- */
static volatile int16_t  enc_pos;   // position in detents
static volatile int8_t   enc_win;   // detents since the last 100 ms task run
static volatile uint16_t enc_miss;  // invalid transitions

/* ---- interrupt only ---- */
static uint8_t enc_prev;            // last CLK/DT state (bit0 = CLK, bit1 = DT)
static int8_t  enc_sub;             // quarter steps since the last detent

/* ---- main only ---- */
static int16_t enc_speed;           // detents/s (signed)

// Quadrature table, index = prev * 4 + now, state = (DT << 1) | CLK.
// +1 / -1 = one quarter step, 0 = no change, 2 = invalid (both pins changed)
static const int8_t qdec[16] = {
     0, +1, -1,  2,
    -1,  0,  2, +1,
    +1,  2,  0, -1,
     2, -1, +1,  0
};

static uint8_t enc_read(void)       // (DT << 1) | CLK
{
    uint8_t v = ENC_PORT;
    return (uint8_t)(v & ENC_MASK);
}

// Called from both the IOC edge interrupt and the 1 ms tick. If the state
// has not changed since the last call, the table gives 0 and nothing happens.
static void enc_decode(void)
{
    uint8_t now = enc_read();
    uint8_t idx = enc_prev;
    idx <<= 2;
    idx |= now;
    int8_t  d   = qdec[idx];
    enc_prev = now;

    if (d == 2)
    {
        enc_miss++;
    }
    else if (d)
    {
        enc_sub = (int8_t)(enc_sub + d);
        if (now == ENC_MASK)            // detent rest position (both high)
        {
            int8_t step = 0;
            if (enc_sub >= 2)       step = 1;    // tolerant of one missed edge
            else if (enc_sub <= -2) step = -1;
            enc_sub = 0;
            if (step)
            {
#if ENCODER_REVERSE
                step = (int8_t)-step;
#endif
                enc_pos += step;
                enc_win  = (int8_t)(enc_win + step);
                LED_TOGGLE();
            }
        }
    }
}

void encoder_init(void)
{
    ENC_ANSEL &= (uint8_t)~ENC_MASK;    // digital
    ENC_TRIS  |= ENC_MASK;              // inputs
    ENC_INLVL |= ENC_MASK;              // Schmitt-trigger input levels
    ENC_WPU   |= ENC_MASK;              // weak pull-ups

    __delay_ms(5);                      // let the pull-ups settle
    enc_prev = enc_read();

    // interrupt-on-change, rising and falling edges
    ENC_IOCP |= ENC_MASK;
    ENC_IOCN |= ENC_MASK;
    ENC_IOCF &= (uint8_t)~ENC_MASK;
    INTCONbits.IOCIE = 1;
}

int16_t encoder_get_pos(void)
{
    int16_t v;
    ATOMIC_BEGIN();
    v = enc_pos;
    ATOMIC_END();
    return v;
}

int16_t encoder_get_speed(void)
{
    return enc_speed;
}

uint16_t encoder_get_miss(void)
{
    uint16_t v;
    ATOMIC_BEGIN();
    v = enc_miss;
    ATOMIC_END();
    return v;
}

void encoder_reset(void)
{
    ATOMIC_BEGIN();
    enc_pos  = 0;
    enc_miss = 0;
    ATOMIC_END();
}

// Speed: sum of the last 5 slots of 100 ms, x2 = detents per second.
void encoder_task_100ms(void)
{
    static uint8_t slot = 0;
    static int8_t  slots[5];            // 5 x 100 ms = 500 ms window
    int8_t w;

    ATOMIC_BEGIN();                     // take and clear in one step
    w = enc_win;
    enc_win = 0;
    ATOMIC_END();

    slots[slot] = w;
    if (++slot >= 5u) slot = 0;

    int16_t sum = 0;
    for (uint8_t i = 0; i < 5u; i++) sum += slots[i];
    enc_speed = (int16_t)(sum * 2);
}

void encoder_isr_edge(void)
{
    ENC_IOCF &= (uint8_t)~ENC_MASK;     // clear first, then read the pins:
    enc_decode();                       // a change after this re-triggers
}

void encoder_isr_tick(void)
{
    enc_decode();
}
