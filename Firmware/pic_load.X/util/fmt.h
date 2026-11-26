/**
 * @file    fmt.h
 * @brief   Small number-to-text helpers (no printf, no hardware access).
 *
 * Plain C, so this module can also be compiled and tested on a PC.
 */
#ifndef FMT_H
#define FMT_H

#include <stdint.h>

/**
 * @brief Unsigned value as zero-padded decimal, e.g. fmt_u(42, 4, b) -> "0042".
 * @param v       value (higher digits than @p digits are dropped)
 * @param digits  number of digits written
 * @param buf     at least digits + 1 chars (NUL terminated)
 */
void fmt_u(uint16_t v, uint8_t digits, char *buf);

/**
 * @brief Signed value as sign + 5 digits, e.g. -12 -> "-00012", 7 -> "+00007".
 * @param buf     at least 7 chars (NUL terminated)
 */
void fmt_s(int16_t v, char *buf);

#endif /* FMT_H */
