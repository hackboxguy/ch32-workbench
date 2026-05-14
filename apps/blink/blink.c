#include "ch32fun.h"
#include "board.h"

/*
 * Blink: toggles the user LED at ~2 Hz.
 *
 * Talks directly to ch32fun's register-level API. The board header is the
 * only place that names a specific port/pin; this file is portable across
 * every board that defines BOARD_LED_*.
 */

#define LED_BIT           (1u << BOARD_LED_PIN)
#define LED_CFG_SHIFT     (BOARD_LED_PIN * 4)
#define LED_CFG_OUTPUT_PP (0x1u << LED_CFG_SHIFT) /* MODE=10MHz, CNF=push-pull */
#define LED_CFG_MASK      (0xFu << LED_CFG_SHIFT)

static inline void led_write(int on) {
    /* BSHR: low 16 bits set the pin, high 16 bits reset it (atomic). */
    if (on) {
        BOARD_LED_PORT->BSHR = LED_BIT;
    } else {
        BOARD_LED_PORT->BSHR = LED_BIT << 16;
    }
}

int main(void) {
    SystemInit();

    /* Enable the GPIO port clock that hosts the LED. */
    RCC->APB2PCENR |= BOARD_LED_RCC_BIT;

    /* Configure the LED pin as 10 MHz push-pull output. */
    BOARD_LED_PORT->CFGLR = (BOARD_LED_PORT->CFGLR & ~LED_CFG_MASK) | LED_CFG_OUTPUT_PP;

    int state = BOARD_LED_ACTIVE_HIGH ? 0 : 1; /* start with LED off */
    led_write(state);

    while (1) {
        state ^= 1;
        led_write(state);
        Delay_Ms(250);
    }
}
