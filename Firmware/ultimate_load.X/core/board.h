/**
 * @file    board.h
 * @brief   Pin map of the Ultimate Load control board (PIC16F1779).
 *
 * Every pin used by the firmware is listed here. Drivers use these macros
 * instead of naming registers directly, so moving a signal to another pin
 * only needs a change in this file.
 *
 * | Signal            | Pin       | Direction | Notes                          |
 * |-------------------|-----------|-----------|--------------------------------|
 * | Encoder CLK       | RA0       | in        | pull-up, Schmitt, IOC          |
 * | Encoder DT        | RA1       | in        | pull-up, Schmitt, IOC          |
 * | Encoder button SW | RA2       | in        | pull-up, Schmitt, low = pressed|
 * | I2C SCL (MSSP1)   | RC3       | open-drain| via PPS, see drivers/i2c.c     |
 * | I2C SDA (MSSP1)   | RC4       | open-drain| via PPS, see drivers/i2c.c     |
 * | Keypad cols 4..1  | RD0..RD3  | out (low) | drive lines, one at a time     |
 * | Keypad rows 4..1  | RD4..RD7  | in        | sense lines, pull-up, Schmitt  |
 *
 * The keypad connector is fitted in reverse pin order (keypad pin 8 -> RD0,
 * pin 1 -> RD7); drivers/keypad.c maps the keys accordingly.
 * | Status LED        | RE2       | out       | toggles on each encoder detent |
 *
 * The GLCD (ST7920) is on the I2C bus through a PCF8574A at address 0x38,
 * see drivers/st7920_glcd_config.h.
 */
#ifndef BOARD_H
#define BOARD_H

#include <xc.h>

/* ---- rotary encoder: CLK = RA0 (bit 0), DT = RA1 (bit 1) ---- */
#define ENC_PORT        PORTA
#define ENC_TRIS        TRISA
#define ENC_ANSEL       ANSELA
#define ENC_WPU         WPUA
#define ENC_INLVL       INLVLA
#define ENC_IOCP        IOCAP
#define ENC_IOCN        IOCAN
#define ENC_IOCF        IOCAF
#define ENC_MASK        0x03u       /* CLK must be bit 0, DT bit 1 */

/* ---- encoder push button: RA2, pressed = low ---- */
#define BTN_PORT        PORTA
#define BTN_TRIS        TRISA
#define BTN_ANSEL       ANSELA
#define BTN_WPU         WPUA
#define BTN_INLVL       INLVLA
#define BTN_MASK        0x04u
#define BTN_IS_PRESSED() ((BTN_PORT & BTN_MASK) == 0u)

/* ---- 4x4 keypad: whole port D, rows RD0..RD3, columns RD4..RD7 ---- */
#define KP_PORT         PORTD
#define KP_TRIS         TRISD
#define KP_LAT          LATD
#define KP_ANSEL        ANSELD
#define KP_WPU          WPUD
#define KP_INLVL        INLVLD
#define KP_ROW_MASK     0x0Fu
#define KP_COL_MASK     0xF0u

/* ---- status LED: RE2 ---- */
#define LED_INIT()      do { ANSELEbits.ANSE2 = 0; LATEbits.LATE2 = 0; TRISEbits.TRISE2 = 0; } while (0)
#define LED_TOGGLE()    (LATEbits.LATE2 ^= 1)

#endif /* BOARD_H */
