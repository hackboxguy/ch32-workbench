#include "ch32fun.h"

/*
 * Diagnostic: figure out which GPIO drives the user LED on an unknown board.
 *
 * Sweeps every usable CH32V003 GPIO. For each pin:
 *   - configures it as 10 MHz push-pull output
 *   - announces "PIN: <name>\r\n" on USART1 (PD5, 115200 8N1)
 *   - blinks the pin (250 ms on / 250 ms off) 4 times = 2 seconds total
 *   - returns the pin to floating input before moving on
 * Then 1 second of silence (all pins low), then the next pin.
 *
 * To identify your LED pin:
 *   - open a serial terminal on the WCH-LinkE's USB-CDC at 115200, OR
 *   - count the bursts: PA1, PA2, PC0..PC7, PD0, PD2..PD7 (PD1=SWIO skipped)
 *
 * The LED will blink during the burst matching its pin. Whatever board
 * polarity (active-high or active-low), you'll see ~4 transitions either
 * way -- the eye doesn't care which phase started high.
 *
 * Total sweep: 17 pins x 3 s = 51 s, then it loops.
 */

typedef struct {
    GPIO_TypeDef *port;
    uint8_t pin;
    uint32_t rcc_bit;
    const char *name;
} pin_t;

static const pin_t pins[] = {
    {GPIOA, 1, RCC_APB2Periph_GPIOA, "PA1"},
    {GPIOA, 2, RCC_APB2Periph_GPIOA, "PA2"},
    {GPIOC, 0, RCC_APB2Periph_GPIOC, "PC0"},
    {GPIOC, 1, RCC_APB2Periph_GPIOC, "PC1"},
    {GPIOC, 2, RCC_APB2Periph_GPIOC, "PC2"},
    {GPIOC, 3, RCC_APB2Periph_GPIOC, "PC3"},
    {GPIOC, 4, RCC_APB2Periph_GPIOC, "PC4"},
    {GPIOC, 5, RCC_APB2Periph_GPIOC, "PC5"},
    {GPIOC, 6, RCC_APB2Periph_GPIOC, "PC6"},
    {GPIOC, 7, RCC_APB2Periph_GPIOC, "PC7"},
    {GPIOD, 0, RCC_APB2Periph_GPIOD, "PD0"},
    /* PD1 is SWIO -- skipping so we don't fight the programmer. */
    {GPIOD, 2, RCC_APB2Periph_GPIOD, "PD2"},
    {GPIOD, 3, RCC_APB2Periph_GPIOD, "PD3"},
    {GPIOD, 4, RCC_APB2Periph_GPIOD, "PD4"},
    {GPIOD, 5, RCC_APB2Periph_GPIOD, "PD5"}, /* also USART1 TX */
    {GPIOD, 6, RCC_APB2Periph_GPIOD, "PD6"},
    {GPIOD, 7, RCC_APB2Periph_GPIOD, "PD7"},
};
#define N_PINS (sizeof(pins) / sizeof(pins[0]))

static void uart_init(void) {
    RCC->APB2PCENR |= RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOD;
    /* PD5 alt-func push-pull, 10 MHz: CFG nibble = 0x9 = (10MHz | AF_PP). */
    GPIOD->CFGLR = (GPIOD->CFGLR & ~(0xFu << (5 * 4))) | (0x9u << (5 * 4));
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

/* Configure pin as 10 MHz push-pull output, leaving other pins in the same
 * port untouched. */
static void pin_output(GPIO_TypeDef *port, int pin) {
    uint32_t shift = (uint32_t)pin * 4;
    /* MODE=01 (10 MHz), CNF=00 (push-pull out) -> CFG nibble = 0x1 */
    port->CFGLR = (port->CFGLR & ~(0xFu << shift)) | (0x1u << shift);
}

/* Return pin to floating input so a temporary drive doesn't fight whatever
 * the board has wired to that pin. */
static void pin_floating_input(GPIO_TypeDef *port, int pin) {
    uint32_t shift = (uint32_t)pin * 4;
    /* MODE=00 (input), CNF=01 (floating input) -> CFG nibble = 0x4 */
    port->CFGLR = (port->CFGLR & ~(0xFu << shift)) | (0x4u << shift);
}

static void pin_set(GPIO_TypeDef *port, int pin, int high) {
    uint32_t bit = 1u << pin;
    port->BSHR = high ? bit : (bit << 16);
}

int main(void) {
    SystemInit();
    uart_init();
    uart_write("\r\n=== pin_sweep starting ===\r\n");

    for (;;) {
        for (unsigned i = 0; i < N_PINS; i++) {
            const pin_t *p = &pins[i];

            uart_write("PIN: ");
            uart_write(p->name);
            uart_write("\r\n");

            RCC->APB2PCENR |= p->rcc_bit;
            pin_output(p->port, p->pin);

            /* 4 toggles, 250 ms each = 2 seconds of clearly visible blinking */
            for (int j = 0; j < 4; j++) {
                pin_set(p->port, p->pin, 1);
                Delay_Ms(250);
                pin_set(p->port, p->pin, 0);
                Delay_Ms(250);
            }

            pin_floating_input(p->port, p->pin);

            /* 1 second of silence between pins so the burst boundary is
             * easy to see. */
            Delay_Ms(1000);
        }
        uart_write("=== sweep complete, restarting ===\r\n");
    }
}
