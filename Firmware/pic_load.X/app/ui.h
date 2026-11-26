/**
 * @file    ui.h
 * @brief   Test screen for the encoder, its button and the keypad (ST7920 GLCD).
 *
 * Screen (128 x 64, graphics mode)
 * @verbatim
 *   ENCODER  K:5 012                [SW]
 *   ------------------------------------
 *    dial      POS  +00012
 *              SPD  010/s  CW
 *              MAX  025/s
 *              RPM  0030
 *             MISS  00000
 *              [=====     speed bar    ]
 * @endverbatim
 *   - dial : needle follows the shaft (ENCODER_DETENTS_PER_REV per turn)
 *   - POS  : position in detents
 *   - SPD  : speed in detents/s (500 ms window) and direction
 *   - MAX  : peak speed since the last reset
 *   - RPM  : shaft speed
 *   - MISS : invalid encoder transitions, should stay 0
 *   - bar  : speed bar, full scale = UI_SPEED_BAR_MAX detents/s
 *   - SW   : box fills while the encoder button is held;
 *            a press resets POS, MAX and MISS
 *   - K:x nnn : last keypad key (inverted while held) and press count
 *               (one press must add exactly 1)
 *
 * Only fields whose value changed are redrawn, and glcd_update() sends only
 * the changed areas of the frame buffer.
 */
#ifndef UI_H
#define UI_H

/** Switch the GLCD to graphics mode and draw the fixed parts.
 *  Call after glcd_init(). */
void ui_init(void);

/** Task: handle the button, redraw changed values, send them to the GLCD.
 *  Run every 20 ms. */
void ui_task(void);

#endif /* UI_H */
