/**
 * @file    ui.c
 * @brief   Test screen for the encoder, its button and the keypad.
 */
#include <stdint.h>
#include "ui.h"
#include "st7920_glcd.h"
#include "encoder.h"
#include "button.h"
#include "keypad.h"
#include "fmt.h"

#define UI_SPEED_BAR_MAX    60      // detents/s for a full speed bar

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

/* ---------------- last shown values ---------------- */
// start "impossible" so everything is drawn once
static int16_t  shown_pos   = 0x7FFF;
static int16_t  shown_spd   = 0x7FFF;
static int16_t  shown_max   = -1;
static uint16_t shown_miss  = 0xFFFF;
static uint8_t  shown_sw    = 0xFF;
static uint8_t  shown_angle = 0xFF;
static uint8_t  shown_bar   = 0xFF;
static char     shown_kkey  = (char)0xFF;
static char     shown_klast = (char)0xFF;
static uint16_t shown_kcnt  = 0xFFFF;
static int16_t  peak        = 0;        // highest |speed| since the last reset

static void dial_needle(uint8_t a, uint8_t color)
{
    int16_t dx = ((int16_t)(DIAL_R - 4) * sin32[(uint8_t)((a + 8u) & 31u)]) / 128;  // cos
    int16_t dy = ((int16_t)(DIAL_R - 4) * sin32[a & 31u]) / 128;                    // sin
    glcd_color(color);
    glcd_line(DIAL_X, DIAL_Y, (uint8_t)(DIAL_X + dx), (uint8_t)(DIAL_Y + dy));
}

static uint8_t dial_angle(int16_t pos)      // 0 = top, clockwise, 32 steps per turn
{
    int16_t m = pos % ENCODER_DETENTS_PER_REV;
    if (m < 0) m += ENCODER_DETENTS_PER_REV;
    return (uint8_t)((24 + (m * 32) / ENCODER_DETENTS_PER_REV) & 31);
}

// clear a text area of 'chars' characters, then write s into it
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

void ui_init(void)
{
    glcd_gfx_begin();
    draw_static();
}

void ui_task(void)
{
    char buf[8];

    // ---- encoder button: reset the counters ----
    if (button_take_press())
    {
        encoder_reset();
        peak = 0;
    }

    // ---- read the inputs ----
    int16_t  pos   = encoder_get_pos();
    int16_t  spd   = encoder_get_speed();
    uint16_t miss  = encoder_get_miss();
    uint8_t  sw    = button_is_down();
    char     kkey  = keypad_get_held();
    char     klast = keypad_get_last();
    uint16_t kcnt  = keypad_get_count();

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

        fmt_u((uint16_t)(((uint32_t)aspd * 60u) / ENCODER_DETENTS_PER_REV), 4, buf);
        field(COL_VAL, ROW_RPM, 4, buf);

        uint16_t bw = ((uint16_t)aspd * (BAR_W - 2u)) / (uint16_t)UI_SPEED_BAR_MAX;
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

    // ---- keypad: last key (inverted while held) and press count ----
    if (kkey != shown_kkey || klast != shown_klast)
    {
        uint8_t box = GLCD_COLOR_OFF, txt = GLCD_COLOR_ON;
        if (kkey) { box = GLCD_COLOR_ON; txt = GLCD_COLOR_OFF; }
        buf[0] = klast ? klast : '-';
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
