# Firmware

The firmware is being rebuilt from scratch, one block at a time. Each step is tested on the
board before the next one is added. The previous firmware is still in the git history (tag
`legacy-firmware`).

## Project

`ultimate_load.X` is an MPLAB X project for the PIC16F1779 (XC8 v1.45, MPLAB X v6.00, internal
oscillator at 16 MHz). Each function has its own header and source file, grouped in folders:

```
ultimate_load.X/
├── main.c            start-up and the task table
├── config_bits.c     configuration words (fuses)
├── app/              application
│   └── ui            test screen for the encoder, button and keypad
├── core/             board and timing
│   ├── board.h       every pin assignment, in one table
│   ├── system        clock, weak pull-ups, ATOMIC_BEGIN / ATOMIC_END
│   ├── tick          1 ms Timer2 time base
│   ├── scheduler     cooperative task table
│   └── isr.c         the single interrupt handler
├── drivers/          hardware drivers
│   ├── encoder       rotary encoder (interrupt-on-change)
│   ├── button        debounced encoder push button
│   ├── keypad        4x4 matrix keypad
│   ├── i2c           MSSP1 I2C master, 400 kHz, bus recovery at start-up
│   └── st7920_glcd   ST7920 128x64 GLCD over a PCF8574A, see below
└── util/             plain C helpers, no hardware access
    └── fmt           number to text without printf
```

The display driver is [st7920-i2c-glcd](https://github.com/naifaljohani/st7920-i2c-glcd).
`drivers/st7920_glcd_config.h` sets its I2C address (0x38, PCF8574A) and expander pin mapping.

Every header starts with a comment block describing the module: what it does, its pins and
timing, and which functions may be called from where.

## How it runs

The firmware uses a 1 ms tick and a table of cooperative tasks. There is no RTOS.

**Interrupts** (`core/isr.c`) do the small jobs that must be on time. They never wait.

| Source | Rate | Work |
|---|---|---|
| Interrupt-on-change, RA0 / RA1 | every edge | encoder decoding |
| Timer2 | 1 ms | time base, encoder backup sample, keypad scan (one row), button debounce |

**Tasks** (table in `main.c`) are run from the main loop by `core/scheduler.c`. Each task runs
to completion and returns; none may wait in a loop or call a long delay.

| Task | Period | Work |
|---|---|---|
| `encoder_task_100ms` | 100 ms | encoder speed over a 500 ms window |
| `ui_task` | 20 ms | encoder button, redraw changed values, send them to the GLCD |

The scheduler is fixed rate: a task that runs late catches up instead of drifting. Data shared
with an interrupt is private to its module and read through getter functions, which turn
interrupts off for a few instructions so multi-byte values are read whole.

**Adding a task:** write a `void my_task(void)` function in its module, declare it in the
header, and add a line `{ my_task, period_ms, 0 }` to the table in `main.c`.

**Adding a source file:** add it to the MPLAB project (in the matching logical folder) so that
it is compiled. Headers in `app/`, `core/`, `drivers/` and `util/` are found through the
project's include paths, so `#include "keypad.h"` works from any file.

## Pins used so far

The full table is in `core/board.h`.

| Pin | Function |
|---|---|
| RA0 / RA1 | Rotary encoder CLK / DT (interrupt-on-change, weak pull-ups, Schmitt inputs) |
| RA2 | Encoder push button (pressed = low, weak pull-up) |
| RC3 / RC4 | I2C SCL / SDA to the GLCD's PCF8574A |
| RD0 – RD3 | Keypad drive lines, driven low one at a time (keypad columns 4 – 1) |
| RD4 – RD7 | Keypad sense lines, weak pull-ups (keypad rows 4 – 1) |
| RE2 | Status LED, toggles on each encoder click |

The keypad's 8-pin connector is fitted in reverse pin order (keypad pin 8 to RD0, pin 1 to RD7).
The key map in `drivers/keypad.c` accounts for this. If the keypad is ever connected the other
way round, only that map needs to change.

## Test screen

| Field | Meaning |
|---|---|
| Dial | Needle follows the encoder shaft |
| POS | Position in clicks |
| SPD | Speed in clicks per second, and direction |
| MAX | Highest speed since the last reset |
| RPM | Shaft speed |
| MISS | Invalid encoder transitions (missed edges), should stay 0 |
| Bar | Speed bar |
| SW | Fills while the encoder button is held. A press resets POS, MAX and MISS |
| `K:x nnn` | Last keypad key (inverted while held) and number of key presses. One press must add exactly 1 |

## Steps

- [x] **1. Display and encoder.** ST7920 128×64 GLCD over I2C, and a rotary encoder decoded with
  interrupt-on-change and a 1 ms Timer2 tick. The screen shows position, speed, RPM and missed
  transitions.
- [x] **2. Keypad.** 4x4 matrix keypad on port D, scanned one row per millisecond with a 20 ms
  debounce. The last key and the press count are shown on the screen.
- [x] **3. Structure.** Code split into modules (`app`, `core`, `drivers`, `util`) and run from a
  1 ms tick and a cooperative task table.
- [ ] 4. Next block

## Building

Open `ultimate_load.X` in MPLAB X and build, or build from the command line inside
`ultimate_load.X`:

```
make -f nbproject/Makefile-default.mk CONF=default build
```

If `make` is not on your PATH, use the one that comes with MPLAB X, for example
`C:\Program Files\Microchip\MPLABX\v6.00\gnuBins\GnuWin32\bin\make.exe`.

The hex file is written to `dist/default/production/ultimate_load.X.production.hex`.

After adding or removing files in MPLAB X, the makefiles in `nbproject/` are regenerated by the
IDE. Commit them with the change so the command-line build keeps working.

## Programming

If MPLAB reports that the low-voltage configuration bit is OFF, untick **Enable Low Voltage
Programming** in Project Properties → PICkit 3 → Program Options and program once with high
voltage. The firmware sets `LVP = ON`, so low-voltage programming works again afterwards.
