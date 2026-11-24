# Firmware

The firmware is being rebuilt from scratch, one block at a time. Each step is tested on the
board before the next one is added. The previous firmware is still in the git history (the
commit before "Firmware: start the rebuild").

## Project

`ultimate_load.X` is an MPLAB X project for the PIC16F1779 (XC8 v1.45, internal oscillator at
16 MHz).

| File | Purpose |
|---|---|
| `main.c` | Current test program |
| `i2c.c`, `i2c.h` | MSSP1 I2C master at 400 kHz, with bus recovery at start-up |
| `st7920_glcd.c`, `st7920_glcd.h`, `st7920_font5x7.h` | [st7920-i2c-glcd](https://github.com/naifaljohani/st7920-i2c-glcd) display driver |
| `st7920_glcd_config.h` | Display settings: I2C address 0x38 (PCF8574A) and expander pin mapping |
| `system.h` | `_XTAL_FREQ` and `xc.h` |

## Pins used so far

| Pin | Function |
|---|---|
| RC3 / RC4 | I2C SCL / SDA to the GLCD's PCF8574A |
| RA0 / RA1 | Rotary encoder CLK / DT (interrupt-on-change, weak pull-ups) |
| RA2 | Encoder push button (pressed = low) |
| RE2 | Status LED, toggles on each encoder click |

## Steps

- [x] **1. Display and encoder.** ST7920 128×64 GLCD over I2C, and a rotary encoder decoded with
  interrupt-on-change and a 1 ms Timer2 tick. The screen shows position, speed, RPM and missed
  transitions.
- [ ] 2. Next block

## Programming

If MPLAB reports that the low-voltage configuration bit is OFF, untick **Enable Low Voltage
Programming** in Project Properties → PICkit 3 → Program Options and program once with high
voltage. The firmware sets `LVP = ON`, so low-voltage programming works again afterwards.
