#include "ch32fun.h"
#include "board.h"

#if !BOARD_HAS_BUTTON
#error "gpio_button requires a board with BOARD_HAS_BUTTON 1"
#endif

/*
 * Mirrors the user button onto the user LED. Debounces in software by
 * requiring two consecutive samples 15 ms apart to agree before flipping
 * the LED.
 *
 * All port/pin specifics come from the selected board header -- this
 * file is identical across all three v1 boards.
 */

#define LED_BIT            (1u << BOARD_LED_PIN)
#define LED_CFG_SHIFT      (BOARD_LED_PIN * 4)
#define LED_CFG_OUTPUT_PP  ((uint32_t)(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << LED_CFG_SHIFT)
#define LED_CFG_MASK       ((uint32_t)0xFu << LED_CFG_SHIFT)

#define BTN_BIT            (1u << BOARD_BUTTON_PIN)
#define BTN_CFG_SHIFT      (BOARD_BUTTON_PIN * 4)
#define BTN_CFG_INPUT_PUPD ((uint32_t)GPIO_CNF_IN_PUPD << BTN_CFG_SHIFT)
#define BTN_CFG_MASK       ((uint32_t)0xFu << BTN_CFG_SHIFT)

static inline void led_write(int on) {
    BOARD_LED_PORT->BSHR = on ? LED_BIT : (LED_BIT << 16);
}

static inline int button_pressed(void) {
    int raw = (BOARD_BUTTON_PORT->INDR >> BOARD_BUTTON_PIN) & 1;
    return BOARD_BUTTON_ACTIVE_LOW ? (raw == 0) : (raw == 1);
}

int main(void) {
    SystemInit();

    /* LED: enable port clock, configure as 10 MHz push-pull output. */
    RCC->APB2PCENR |= BOARD_LED_RCC_BIT;
    BOARD_LED_PORT->CFGLR = (BOARD_LED_PORT->CFGLR & ~LED_CFG_MASK) | LED_CFG_OUTPUT_PP;

    /* Button: enable port clock, configure as input with pull-up/down. */
    RCC->APB2PCENR |= BOARD_BUTTON_RCC_BIT;
    BOARD_BUTTON_PORT->CFGLR = (BOARD_BUTTON_PORT->CFGLR & ~BTN_CFG_MASK) | BTN_CFG_INPUT_PUPD;

    /* For input-with-pull-up/down (CNF=IN_PUPD), the ODR bit selects pull
     * direction: 1 = pull-up, 0 = pull-down. Active-low buttons need a
     * pull-up so the line idles high. */
    if (BOARD_BUTTON_ACTIVE_LOW) {
        BOARD_BUTTON_PORT->BSHR = BTN_BIT; /* ODR bit set: pull-up */
    } else {
        BOARD_BUTTON_PORT->BSHR = BTN_BIT << 16; /* ODR bit clear: pull-down */
    }

    led_write(0);

    int stable_state = 0;
    while (1) {
        int s1 = button_pressed();
        Delay_Ms(15);
        int s2 = button_pressed();
        if (s1 == s2 && s1 != stable_state) {
            stable_state = s1;
            led_write(BOARD_LED_ACTIVE_HIGH ? stable_state : !stable_state);
        }
    }
}
