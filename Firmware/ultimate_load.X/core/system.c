/**
 * @file    system.c
 * @brief   Clock and basic board setup.
 */
#include "system.h"
#include "board.h"

void system_init(void)
{
    OSCCON = 0x7A;              // IRCF = 1111 (16 MHz HF), SCS = 1x (internal osc)

    LED_INIT();                 // status LED off, output

    // weak pull-ups: all off here, each driver enables its own pins
    WPUA = 0x00;
    WPUB = 0x00;
    WPUC = 0x00;
    WPUD = 0x00;
    WPUE = 0x00;
    OPTION_REGbits.nWPUEN = 0;  // master enable for the WPUx registers
}
