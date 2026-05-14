#include "ch32fun.h"

/*
 * Diagnostic: figure out which GPIO drives the user LED on an unknown board.
 *
 * Uses ch32fun's funapi (funGpioInitAll, funPinMode, funDigitalWrite) -- the
 * same code path that ch32fun's own examples/blink uses and that has been
 * known to work on the F4P6 dev board. The earlier register-direct version
 * of this file produced no visible blink on at least one V1772 board; the
 * cause was never narrowed down. Switching to the helper API removes any
 * chance of a subtle raw-register bug being the reason.
 *
 * Per pin: announce "PIN: <name>\r\n" on USART1 (PD5, 115200 8N1), blink
 * the pin 4 times at 250 ms cadence (2 s total), then 1 s of silence.
 *
 * Identifying your LED pin:
 *   - watch the serial monitor (115200 8N1 on the WCH-LinkE's USB-CDC) and
 *     wait for the LED to come alive during a "PIN: PXn" line, OR
 *   - count visual bursts: PA1, PA2, PC0..PC7, PD0, PD2..PD7
 *     (PD1=SWIO skipped to avoid fighting the programmer).
 *
 * If neither the LED ever lights NOR the UART output ever appears, the
 * chip isn't running -- check power, the programmer, the flash log.
 */

static const struct {
    uint32_t pin;
    const char *name;
} pins[] = {
    {PA1, "PA1"},
    {PA2, "PA2"},
    {PC0, "PC0"},
    {PC1, "PC1"},
    {PC2, "PC2"},
    {PC3, "PC3"},
    {PC4, "PC4"},
    {PC5, "PC5"},
    {PC6, "PC6"},
    {PC7, "PC7"},
    {PD0, "PD0"},
    /* PD1 is SWIO -- skipping. */
    {PD2, "PD2"},
    {PD3, "PD3"},
    {PD4, "PD4"},
    {PD5, "PD5"}, /* shared with USART1 TX */
    {PD6, "PD6"},
    {PD7, "PD7"},
};
#define N_PINS (sizeof(pins) / sizeof(pins[0]))

static void uart_init(void) {
    RCC->APB2PCENR |= RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOD;
    /* PD5 as alt-function push-pull (USART1 TX). */
    funPinMode(PD5, GPIO_CFGLR_OUT_10Mhz_AF_PP);
    USART1->BRR = (FUNCONF_SYSTEM_CORE_CLOCK + 115200 / 2) / 115200;
    USART1->CTLR1 = USART_CTLR1_TE | USART_CTLR1_UE;
}

static void uart_write(const char *s) {
    while (*s) {
        while (!(USART1->STATR & USART_FLAG_TXE)) {
        }
        USART1->DATAR = (uint16_t)(*s++);
    }
}

int main(void) {
    SystemInit();

    /* Enable every GPIO port clock at once -- matches last year's known-
     * good blink.c that toggled PD0/PD4/PD6/PC0 on this board. */
    funGpioInitAll();

    uart_init();
    uart_write("\r\n=== pin_sweep (funapi) starting ===\r\n");

    /* Configure every sweep pin as a 10 MHz push-pull output upfront, so
     * we don't pay the reconfigure cost inside the burst loop. */
    for (unsigned i = 0; i < N_PINS; i++) {
        funPinMode(pins[i].pin, GPIO_CFGLR_OUT_10Mhz_PP);
        funDigitalWrite(pins[i].pin, FUN_LOW);
    }

    for (;;) {
        for (unsigned i = 0; i < N_PINS; i++) {
            uart_write("PIN: ");
            uart_write(pins[i].name);
            uart_write("\r\n");

            /* 4 blinks at 250 ms cadence = 2 seconds clearly visible. */
            for (int j = 0; j < 4; j++) {
                funDigitalWrite(pins[i].pin, FUN_HIGH);
                Delay_Ms(250);
                funDigitalWrite(pins[i].pin, FUN_LOW);
                Delay_Ms(250);
            }

            /* 1 second of silence before the next pin so the burst
             * boundary is obvious. */
            Delay_Ms(1000);
        }
        uart_write("=== sweep complete, restarting ===\r\n");
    }
}
