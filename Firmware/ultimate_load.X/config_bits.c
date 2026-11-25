/**
 * @file    config_bits.c
 * @brief   PIC16F1779 configuration words (fuses).
 */
#include <xc.h>

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
