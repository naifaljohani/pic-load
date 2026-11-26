/**
 * @file    fmt.c
 * @brief   Small number-to-text helpers.
 */
#include "fmt.h"

void fmt_u(uint16_t v, uint8_t digits, char *buf)
{
    for (int8_t i = (int8_t)(digits - 1u); i >= 0; i--)
    {
        buf[i] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    buf[digits] = '\0';
}

void fmt_s(int16_t v, char *buf)
{
    buf[0] = (v < 0) ? '-' : '+';
    fmt_u((uint16_t)((v < 0) ? -v : v), 5, buf + 1);
}
