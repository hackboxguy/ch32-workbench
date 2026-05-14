#include "ch32fun.h"

/*
 * Sanity-check copy of the user's known-good last-year blink.c, with the
 * delay raised from 2 ms (invisible PWM) to 250 ms (clearly visible 2 Hz).
 * Drives PD0, PD4, PD6, PC0 simultaneously -- the four candidates from
 * tmp-debug/blink.c -- so whichever of them is wired to the LED on this
 * board will flash.
 *
 * If THIS one doesn't blink, the chip itself is not running and the
 * problem is upstream of any of our code (programmer, power, BOOT0).
 */

#define PIN_1     PD0
#define PIN_K     PD4
#define PIN_BOB   PD6
#define PIN_KEVIN PC0

int main(void) {
    SystemInit();
    funGpioInitAll();

    funPinMode(PIN_1, GPIO_CFGLR_OUT_10Mhz_PP);
    funPinMode(PIN_K, GPIO_CFGLR_OUT_10Mhz_PP);
    funPinMode(PIN_BOB, GPIO_CFGLR_OUT_10Mhz_PP);
    funPinMode(PIN_KEVIN, GPIO_CFGLR_OUT_10Mhz_PP);

    while (1) {
        funDigitalWrite(PIN_1, FUN_HIGH);
        funDigitalWrite(PIN_K, FUN_HIGH);
        funDigitalWrite(PIN_BOB, FUN_HIGH);
        funDigitalWrite(PIN_KEVIN, FUN_HIGH);
        Delay_Ms(250);
        funDigitalWrite(PIN_1, FUN_LOW);
        funDigitalWrite(PIN_K, FUN_LOW);
        funDigitalWrite(PIN_BOB, FUN_LOW);
        funDigitalWrite(PIN_KEVIN, FUN_LOW);
        Delay_Ms(250);
    }
}
