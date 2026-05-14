/*
 * SSD1306 I2C OLED demo: pattern intro + running clock.
 *
 * Ported from ch32fun's examples/i2c_oled (E. Brombaugh, 2023-03-29). On
 * boot the app cycles ONCE through 9 graphics demo modes (2 s each), then
 * settles into a permanent clock display counting up from 00:00:00 at
 * roughly 1 Hz.
 *
 * Pattern intro modes:
 *   0: binary buffer fill
 *   1: pixel plot
 *   2: line plot
 *   3: empty + filled circles
 *   4: image (bomb)
 *   5: unscaled text
 *   6: scaled text 8x8 + 16x16
 *   7: scaled text 32x32
 *   8: scaled text 64x64 (128x64 OLEDs only)
 *
 * Clock note: 1 Hz cadence comes from Delay_Ms(1000) plus a few ms of
 * I2C-refresh overhead per tick. That's a real-world drift of a few ms
 * per second, so don't expect this to keep wall-clock time -- it's a
 * demo, not an RTC. After ~24 h it rolls 99:59:59 -> 00:00:00 because
 * the hour counter is two-digit; that's fine for the demo intent.
 *
 * Wiring (CH32V003 hardware I2C1):
 *   SCL: PC2
 *   SDA: PC1
 *   VCC: 3V3
 *   GND: GND
 *
 * Driver lives in ch32fun/extralibs/, auto-added to the include path by
 * ch32fun.mk.
 */

/* OLED variant -- uncomment exactly one. The bomb image expects 128x64. */
// #define SSD1306_64X32
// #define SSD1306_128X32
#define SSD1306_128X64

#include "ch32fun.h"
#include <stdio.h>
#include "ssd1306_i2c.h"
#include "ssd1306.h"

#include "bomb.h"

int main(void) {
    /* 48 MHz internal clock */
    SystemInit();

    Delay_Ms(100);
    printf("\r\r\n\ni2c_ssd1306_oled_test\n\r");

    Delay_Ms(100); /* give the OLED a moment after power-up */
    printf("initializing i2c oled...");

    if (ssd1306_i2c_init()) {
        printf("failed.\n\r");
        printf("Stuck here forever...\n\r");
        while (1) {
        }
    }

    ssd1306_init();
    printf("done.\n\r");
    printf("Running pattern intro (once), then clock...\n\r");

    const uint8_t modes = (SSD1306_H > 32) ? 9 : 8;
    /* ONE pass through the demo modes, then fall through to the clock. */
    for (uint8_t mode = 0; mode < modes; mode++) {
        ssd1306_setbuf(0);

        switch (mode) {
        case 0:
            printf("buffer fill with binary\n\r");
            for (unsigned i = 0; i < sizeof(ssd1306_buffer); i++) {
                ssd1306_buffer[i] = (uint8_t)i;
            }
            break;

        case 1:
            printf("pixel plots\n\r");
            for (int i = 0; i < SSD1306_W; i++) {
                ssd1306_drawPixel(i, i / (SSD1306_W / SSD1306_H), 1);
                ssd1306_drawPixel(i, SSD1306_H - 1 - (i / (SSD1306_W / SSD1306_H)), 1);
            }
            break;

        case 2: {
            printf("line plots\n\r");
            uint8_t y = 0;
            for (uint8_t x = 0; x < SSD1306_W; x += 16) {
                ssd1306_drawLine(x, 0, SSD1306_W, y, 1);
                ssd1306_drawLine(SSD1306_W - x, SSD1306_H, 0, SSD1306_H - y, 1);
                y += SSD1306_H / 8;
            }
            break;
        }

        case 3:
            printf("circles empty and filled\n\r");
            for (uint8_t x = 0; x < SSD1306_W; x += 16) {
                if (x < 64) {
                    ssd1306_drawCircle(x, SSD1306_H / 2, 15, 1);
                } else {
                    ssd1306_fillCircle(x, SSD1306_H / 2, 15, 1);
                }
            }
            break;

        case 4:
            printf("image\n\r");
            ssd1306_drawImage(0, 0, bomb_i_stripped, 32, 32, 0);
            break;

        case 5:
            printf("unscaled text\n\r");
            ssd1306_drawstr(0, 0, "This is a test", 1);
            ssd1306_drawstr(0, 8, "of the emergency", 1);
            ssd1306_drawstr(0, 16, "broadcasting", 1);
            ssd1306_drawstr(0, 24, "system.", 1);
            if (SSD1306_H > 32) {
                ssd1306_drawstr(0, 32, "Lorem ipsum", 1);
                ssd1306_drawstr(0, 40, "dolor sit amet,", 1);
                ssd1306_drawstr(0, 48, "consectetur", 1);
                ssd1306_drawstr(0, 56, "adipiscing", 1);
            }
            ssd1306_xorrect(SSD1306_W / 2, 0, SSD1306_W / 2, SSD1306_W);
            break;

        case 6:
            printf("scaled text 1, 2\n\r");
            ssd1306_drawstr_sz(0, 0, "sz 8x8", 1, fontsize_8x8);
            ssd1306_drawstr_sz(0, 16, "16x16", 1, fontsize_16x16);
            break;

        case 7:
            printf("scaled text 4\n\r");
            ssd1306_drawstr_sz(0, 0, "32x32", 1, fontsize_32x32);
            break;

        case 8:
            printf("scaled text 8\n\r");
            ssd1306_drawstr_sz(0, 0, "64", 1, fontsize_64x64);
            break;

        default:
            break;
        }

        ssd1306_refresh();
        Delay_Ms(2000);
    }

    /* --- Clock phase: count up from 00:00:00, ~1 Hz --- */
    printf("Pattern intro done. Entering clock loop.\n\r");

    /* "HH:MM:SS" -- 8 chars at fontsize_16x16 fills a 128-wide display
     * exactly. y=24 vertically centers it on a 64-tall panel. */
    char buf[9] = "00:00:00";
    unsigned ss = 0, mm = 0, hh = 0;

    while (1) {
        buf[0] = (char)('0' + (hh / 10));
        buf[1] = (char)('0' + (hh % 10));
        buf[3] = (char)('0' + (mm / 10));
        buf[4] = (char)('0' + (mm % 10));
        buf[6] = (char)('0' + (ss / 10));
        buf[7] = (char)('0' + (ss % 10));

        ssd1306_setbuf(0);
        ssd1306_drawstr_sz(0, 24, buf, 1, fontsize_16x16);
        ssd1306_refresh();

        Delay_Ms(1000);

        if (++ss >= 60) {
            ss = 0;
            if (++mm >= 60) {
                mm = 0;
                if (++hh >= 100) {
                    hh = 0; /* rolls 99:59:59 -> 00:00:00; demo-only */
                }
            }
        }
    }
}
