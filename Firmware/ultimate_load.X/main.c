/*
 * Rotary encoder test on the ST7920 GLCD
 * Board : Ultimate Load control board, PIC16F1779
 * I2C   : MSSP1, SCL = RC3, SDA = RC4, 400 kHz (GLCD through PCF8574A)
 * Enc   : CLK = RA0, DT = RA1, SW = RA2 (pressed = low)
 *
 * PIC16F1779 hardware used
 *   - Interrupt-on-change (IOC) on RA0/RA1, both edges: every quadrature
 *     edge is caught in hardware while the CPU is busy talking I2C.
 *   - Timer2 (HLT timer) 1 ms tick: button debounce, speed measurement,
 *     and a backup sample of the encoder pins.
 *   - Weak pull-ups and Schmitt-trigger input levels on RA0..RA2.
 *
 * Screen
 *   left : dial, the needle follows the shaft (ENC_DETENTS_PER_REV)
 *   POS  : position in detents
 *   SPD  : speed in detents/s (500 ms window) and direction
 *   MAX  : peak speed since the last reset
 *   RPM  : shaft speed
 *   MISS : invalid transitions (an edge was missed: shows the limit of
 *          responsiveness, should stay 0 in normal use)
 *   bar  : speed bar, full scale = SPEED_BAR_MAX detents/s
 *   SW   : box fills while the button is held; a press resets POS/MAX/MISS
 *
 * Keypad test (4x4 matrix on port D)
 *   Rows    : RD0..RD3 (row 1 = RD0), driven low one at a time
 *   Columns : RD4..RD7 (col 1 = RD4), inputs with weak pull-ups
 *   Header "K:x nnn" : x = last key (inverted while held),
 *                      nnn = press count (one press must add exactly 1)
 *
 * LED on RE2 toggles on every detent.
 * Device: PIC16F1779, XC8, internal oscillator 16 MHz
 */

// CONFIG1
#pragma config FOSC = INTOSC    // Internal oscillator, I/O on CLKIN pin
#pragma config WDTE = OFF       // Watchdog timer disabled
#pragma config PWRTE = ON       // Power-up timer enabled
#pragma config MCLRE = ON       // MCLR pin is MCLR
#pragma config CP = OFF         // Code protection off
#pragma config BOREN = ON       // Brown-out reset enabled
#pragma config CLKOUTEN = OFF   // CLKOUT disabled
#pragma config IESO = OFF
#pragma config FCMEN = OFF

// CONFIG2
#pragma config WRT = OFF
#pragma config PPS1WAY = OFF
#pragma config ZCD = OFF
#pragma config PLLEN = OFF
#pragma config STVREN = ON
#pragma config BORV = LO
#pragma config LPBOR = OFF
#pragma config LVP = ON         // Low-voltage programming (needed by on-board programmers)

#include "system.h"
#include "i2c.h"
#include "st7920_glcd.h"

/* ---------------- settings ---------------- */
#define ENC_DETENTS_PER_REV 20      // KY-040 style encoders: 20 clicks per turn
#define ENC_REVERSE         0       // 1 = swap the counting direction
#define SPEED_BAR_MAX       60      // detents/s for a full speed bar

/* ---------------- shared with the interrupt ---------------- */
static volatile int16_t  enc_pos;       // position in detents
static volatile int8_t   enc_win;       // detents in the current 100 ms slot
static volatile int16_t  enc_speed;     // detents/s (signed)
static volatile uint16_t enc_miss;      // invalid transitions
static volatile uint8_t  sw_down;       // 1 while the button is held (debounced)
static volatile uint8_t  sw_event;      // set on each new press

static uint8_t enc_prev;                // last CLK/DT state (bit0 = CLK, bit1 = DT)
static int8_t  enc_sub;                 // quarter steps since the last detent

// Quadrature table, index = prev * 4 + now, state = (DT << 1) | CLK.
// +1 / -1 = one quarter step, 0 = no change, 2 = invalid (both pins changed)
static const int8_t qdec[16] = {
     0, +1, -1,  2,
    -1,  0,  2, +1,
    +1,  2,  0, -1,
     2, -1, +1,  0
};

static uint8_t enc_read(void)           // (DT << 1) | CLK  =  RA1:RA0
{
    uint8_t v = PORTA;
    return (uint8_t)(v & 0x03u);
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
        if (now == 0x03u)               // detent rest position (both high)
        {
            int8_t step = 0;
            if (enc_sub >= 2)       step = 1;    // tolerant of one missed edge
            else if (enc_sub <= -2) step = -1;
            enc_sub = 0;
            if (step)
            {
#if ENC_REVERSE
                step = (int8_t)-step;
#endif
                enc_pos += step;
                enc_win  = (int8_t)(enc_win + step);
                LATEbits.LATE2 ^= 1;
            }
        }
    }
}

/* ---------------- 4x4 keypad on port D ---------------- */
#define KP_DEBOUNCE  5                  // equal full scans (5 x 4 ms = 20 ms)

// key at row r, column c = kp_map[r * 4 + c]
static const char kp_map[] = "123A456B789C*0#D";

static volatile uint8_t  kp_key;        // key held now (debounced), 0 = none
static volatile uint8_t  kp_last;       // last key pressed, 0 = none yet
static volatile uint16_t kp_count;      // number of presses

static uint8_t kp_row;                  // row being driven low (0..3)

static void kp_select_row(uint8_t r)    // selected row = output low, rest = inputs
{
    TRISD = (uint8_t)~(1u << r);
}

// Called every 1 ms from the tick. The columns are read for the row that was
// selected on the previous tick (1 ms to settle), then the next row is selected.
// A full scan of the 4 rows takes 4 ms.
static void kp_scan(void)
{
    static uint8_t found = 0;           // key seen during the current scan
    static uint8_t cand  = 0;           // debounce candidate
    static uint8_t cnt   = 0;

    uint8_t cols = (uint8_t)(((uint8_t)~PORTD) >> 4);   // pressed = 1, bit0 = RD4
    if (cols && !found)
    {
        uint8_t c = 0;
        while (!(cols & 1u)) { cols >>= 1; c++; }
        found = (uint8_t)kp_map[(uint8_t)(kp_row * 4u + c)];
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

void interrupt isr(void)
{
    /* ---- encoder edge: IOC on RA0 (CLK) / RA1 (DT), both edges ---- */
    if (INTCONbits.IOCIF)
    {
        IOCAF &= 0xFC;                  // clear first, then read the pins:
        enc_decode();                   // a change after this re-triggers
    }

    /* ---- 1 ms tick (Timer2) ---- */
    if (PIR1bits.TMR2IF)
    {
        static uint8_t sw_hist = 0;
        static uint8_t ms      = 0;
        static uint8_t slot    = 0;
        static int8_t  slots[5];        // 5 x 100 ms = 500 ms speed window

        PIR1bits.TMR2IF = 0;

        enc_decode();                   // backup sample (catches anything missed)
        kp_scan();                      // keypad: one row per tick

        // button: 8 equal samples in a row = debounced
        sw_hist = (uint8_t)(sw_hist << 1) | (PORTAbits.RA2 ? 0u : 1u);
        if (sw_hist == 0xFFu && !sw_down) { sw_down = 1; sw_event = 1; }
        else if (sw_hist == 0x00u)        { sw_down = 0; }

        // speed: sum of the last 5 slots of 100 ms, x2 = detents per second
        if (++ms >= 100u)
        {
            ms = 0;
            slots[slot] = enc_win;
            enc_win = 0;
            if (++slot >= 5u) slot = 0;
            int16_t sum = 0;
            for (uint8_t i = 0; i < 5u; i++) sum += slots[i];
            enc_speed = (int16_t)(sum * 2);
        }
    }
}

static void encoder_init(void)
{
    ANSELA &= 0xF8;                 // RA0..RA2 digital
    TRISA  |= 0x07;                 // inputs
    INLVLA |= 0x07;                 // Schmitt-trigger input levels
    WPUA    = 0x07;                 // weak pull-ups on RA0..RA2 only
    WPUB    = 0x00;
    WPUC    = 0x00;
    WPUD    = 0x00;
    WPUE    = 0x00;
    OPTION_REGbits.nWPUEN = 0;      // enable weak pull-ups

    __delay_ms(5);                  // let the pull-ups settle
    enc_prev = enc_read();

    // interrupt-on-change, rising and falling edges on RA0 / RA1
    IOCAP |= 0x03;
    IOCAN |= 0x03;
    IOCAF  = 0x00;
    INTCONbits.IOCIE = 1;
}

// Call after encoder_init() (it clears WPUD) and before tick_init().
static void keypad_init(void)
{
    ANSELD = 0x00;                  // port D digital
    LATD   = 0x00;                  // selected row drives low
    INLVLD |= 0xF0;                 // Schmitt-trigger levels on the columns
    WPUD   = 0xF0;                  // weak pull-ups on the columns RD4..RD7
    kp_row = 0;
    TRISD  = 0xFE;                  // row 1 (RD0) selected, rest inputs
}

static void tick_init(void)
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

/* ---------------- small text helpers (no printf) ---------------- */
static void fmt_u(uint16_t v, uint8_t digits, char *buf)   // zero padded
{
    for (int8_t i = (int8_t)(digits - 1u); i >= 0; i--)
    {
        buf[i] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    buf[digits] = '\0';
}

static void fmt_s(int16_t v, char *buf)                     // "-00012" / "+00012"
{
    buf[0] = (v < 0) ? '-' : '+';
    fmt_u((uint16_t)((v < 0) ? -v : v), 5, buf + 1);
}

/* ---------------- dial ---------------- */
#define DIAL_X  21
#define DIAL_Y  37
#define DIAL_R  19

// sin(i * 360/32 degrees) * 127, i = 0..31
static const int8_t sin32[32] = {
       0,   25,   49,   71,   90,  106,  117,  125,
     127,  125,  117,  106,   90,   71,   49,   25,
       0,  -25,  -49,  -71,  -90, -106, -117, -125,
    -127, -125, -117, -106,  -90,  -71,  -49,  -25
};

static void dial_needle(uint8_t a, uint8_t color)
{
    int16_t dx = ((int16_t)(DIAL_R - 4) * sin32[(uint8_t)((a + 8u) & 31u)]) / 128;  // cos
    int16_t dy = ((int16_t)(DIAL_R - 4) * sin32[a & 31u]) / 128;                    // sin
    glcd_color(color);
    glcd_line(DIAL_X, DIAL_Y, (uint8_t)(DIAL_X + dx), (uint8_t)(DIAL_Y + dy));
}

static uint8_t dial_angle(int16_t pos)      // 0 = top, clockwise, 32 steps per turn
{
    int16_t m = pos % ENC_DETENTS_PER_REV;
    if (m < 0) m += ENC_DETENTS_PER_REV;
    return (uint8_t)((24 + (m * 32) / ENC_DETENTS_PER_REV) & 31);
}

/* ---------------- screen layout ---------------- */
#define COL_LBL  46
#define COL_VAL  70
#define ROW_POS  13
#define ROW_SPD  22
#define ROW_MAX  31
#define ROW_RPM  40
#define ROW_MISS 49
#define BAR_X    46
#define BAR_Y    58
#define BAR_W    81
#define BAR_H    6

static void field(uint8_t x, uint8_t y, uint8_t chars, const char *s)
{
    glcd_color(GLCD_COLOR_OFF);
    glcd_fill_rect(x, y, (uint8_t)(chars * 6u), 8);
    glcd_color(GLCD_COLOR_ON);
    glcd_text(x, y, s);
}

static void draw_static(void)
{
    glcd_fb_clear();
    glcd_color(GLCD_COLOR_ON);
    glcd_text(2, 1, "ENCODER");
    glcd_text(50, 1, "K:");                                         // keypad test
    glcd_line(0, 10, 127, 10);

    glcd_circle(DIAL_X, DIAL_Y, DIAL_R);
    glcd_fill_circle(DIAL_X, DIAL_Y, 2);
    glcd_vline(DIAL_X, (uint8_t)(DIAL_Y - DIAL_R), 3);              // 0 mark

    glcd_text(COL_LBL, ROW_POS,  "POS");
    glcd_text(COL_LBL, ROW_SPD,  "SPD");
    glcd_text(COL_LBL, ROW_MAX,  "MAX");
    glcd_text(COL_LBL, ROW_RPM,  "RPM");
    glcd_text(COL_LBL - 6, ROW_MISS, "MISS");

    glcd_rect(BAR_X, BAR_Y, BAR_W, BAR_H);
    glcd_rect(108, 0, 19, 10);                                      // SW box
}

void main(void)
{
    OSCCON = 0x7A;              // IRCF = 1111 (16 MHz HF), SCS = 1x (internal osc)

    ANSELEbits.ANSE2 = 0;       // RE2 digital
    TRISEbits.TRISE2 = 0;       // RE2 output (detent LED)
    LATEbits.LATE2 = 0;

    __delay_ms(100);            // let the GLCD and expander settle
    i2c_init();                 // SCL = RC3, SDA = RC4 (frees a stuck bus first)
    glcd_init();
    glcd_gfx_begin();
    draw_static();

    encoder_init();
    keypad_init();              // after encoder_init (WPUD)
    tick_init();
    INTCONbits.GIE = 1;

    // last shown values, start "impossible" so everything is drawn once
    int16_t  shown_pos   = 0x7FFF;
    int16_t  shown_spd   = 0x7FFF;
    int16_t  shown_max   = -1;
    uint16_t shown_miss  = 0xFFFF;
    uint8_t  shown_sw    = 0xFF;
    uint8_t  shown_angle = 0xFF;
    uint8_t  shown_bar   = 0xFF;
    int16_t  peak        = 0;
    uint8_t  shown_kkey  = 0xFF;
    uint8_t  shown_klast = 0xFF;
    uint16_t shown_kcnt  = 0xFFFF;
    char     buf[8];

    while (1)
    {
        // ---- snapshot of the interrupt data ----
        INTCONbits.GIE = 0;
        if (sw_event)                       // press: reset counters
        {
            sw_event = 0;
            enc_pos  = 0;
            enc_miss = 0;
            peak     = 0;
        }
        int16_t  pos  = enc_pos;
        int16_t  spd  = enc_speed;
        uint16_t miss = enc_miss;
        uint8_t  sw   = sw_down;
        uint8_t  kkey  = kp_key;
        uint8_t  klast = kp_last;
        uint16_t kcnt  = kp_count;
        INTCONbits.GIE = 1;

        int16_t aspd = (spd < 0) ? (int16_t)-spd : spd;
        if (aspd > peak) peak = aspd;

        // ---- redraw only what changed ----
        if (pos != shown_pos)
        {
            fmt_s(pos, buf);
            field(COL_VAL, ROW_POS, 6, buf);

            uint8_t a = dial_angle(pos);
            if (a != shown_angle)
            {
                if (shown_angle != 0xFF) dial_needle(shown_angle, GLCD_COLOR_OFF);
                dial_needle(a, GLCD_COLOR_ON);
                glcd_fill_circle(DIAL_X, DIAL_Y, 2);
                shown_angle = a;
            }
            shown_pos = pos;
        }

        if (spd != shown_spd)
        {
            fmt_u((uint16_t)aspd, 3, buf);
            buf[3] = '/'; buf[4] = 's'; buf[5] = '\0';
            field(COL_VAL, ROW_SPD, 5, buf);
            field(106, ROW_SPD, 3, (spd > 0) ? "CW " : (spd < 0) ? "CCW" : "   ");

            fmt_u((uint16_t)(((uint32_t)aspd * 60u) / ENC_DETENTS_PER_REV), 4, buf);
            field(COL_VAL, ROW_RPM, 4, buf);

            uint16_t bw = ((uint16_t)aspd * (BAR_W - 2u)) / (uint16_t)SPEED_BAR_MAX;
            if (bw > BAR_W - 2u) bw = BAR_W - 2u;
            uint8_t w = (uint8_t)bw;
            if (w != shown_bar)
            {
                glcd_color(GLCD_COLOR_OFF);
                glcd_fill_rect(BAR_X + 1, BAR_Y + 1, BAR_W - 2, BAR_H - 2);
                glcd_color(GLCD_COLOR_ON);
                if (w) glcd_fill_rect(BAR_X + 1, BAR_Y + 1, w, BAR_H - 2);
                shown_bar = w;
            }
            shown_spd = spd;
        }

        if (peak != shown_max)
        {
            fmt_u((uint16_t)peak, 3, buf);
            buf[3] = '/'; buf[4] = 's'; buf[5] = '\0';
            field(COL_VAL, ROW_MAX, 5, buf);
            shown_max = peak;
        }

        if (miss != shown_miss)
        {
            fmt_u(miss, 5, buf);
            field(COL_VAL, ROW_MISS, 5, buf);
            shown_miss = miss;
        }

        if (sw != shown_sw)             // held = filled box with light text
        {
            uint8_t box = GLCD_COLOR_OFF, txt = GLCD_COLOR_ON;
            if (sw) { box = GLCD_COLOR_ON; txt = GLCD_COLOR_OFF; }
            glcd_color(box);
            glcd_fill_rect(109, 1, 17, 8);
            glcd_color(txt);
            glcd_text(112, 1, "SW");
            shown_sw = sw;
        }

        // ---- keypad test: last key (inverted while held) and press count ----
        if (kkey != shown_kkey || klast != shown_klast)
        {
            uint8_t box = GLCD_COLOR_OFF, txt = GLCD_COLOR_ON;
            if (kkey) { box = GLCD_COLOR_ON; txt = GLCD_COLOR_OFF; }
            buf[0] = klast ? (char)klast : '-';
            buf[1] = '\0';
            glcd_color(box);
            glcd_fill_rect(61, 1, 7, 8);
            glcd_color(txt);
            glcd_text(62, 1, buf);
            shown_kkey  = kkey;
            shown_klast = klast;
        }

        if (kcnt != shown_kcnt)
        {
            fmt_u((uint16_t)(kcnt % 1000u), 3, buf);
            field(72, 1, 3, buf);
            shown_kcnt = kcnt;
        }

        glcd_update();          // sends only the changed areas
    }
}
