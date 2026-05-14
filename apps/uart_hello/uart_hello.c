#include "ch32fun.h"
#include "board.h"

/*
 * Sends "Hello from <BOARD_NAME> on <BOARD_MCU>\r\n" once per second on
 * USART1 at BOARD_UART_BAUD, 8N1, polled (no DMA, no printf).
 *
 * USART1 TX is on the pin declared by BOARD_UART_TX_*. On CH32V003 the
 * default mapping is PD5; alternate mappings exist (PD0, PC0) but the
 * v1 boards all use the default. Boards needing alt mappings will need
 * an extra BOARD_UART_REMAP macro -- deferred.
 */

#define UART_PIN_CFG_AF_PP ((uint32_t)(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF))
#define UART_PIN_SHIFT     (BOARD_UART_TX_PIN * 4)
#define UART_PIN_MASK      ((uint32_t)0xFu << UART_PIN_SHIFT)

static void uart_init(uint32_t baud) {
    /* Enable clocks: USART1 (always on APB2 on CH32V003), and the GPIO
     * port that hosts the TX pin. The two RCC bits may or may not refer
     * to the same port -- ORing both is safe either way. */
    RCC->APB2PCENR |= RCC_APB2Periph_USART1 | BOARD_UART_TX_RCC_BIT;

    /* Configure TX pin as 10 MHz alt-function push-pull. */
    BOARD_UART_TX_PORT->CFGLR =
        (BOARD_UART_TX_PORT->CFGLR & ~UART_PIN_MASK) | (UART_PIN_CFG_AF_PP << UART_PIN_SHIFT);

    /* Baud rate divisor: APB clock / baud, rounded. */
    USART1->BRR = (FUNCONF_SYSTEM_CORE_CLOCK + (baud / 2)) / baud;

    /* 8N1 is the reset default; just enable TX and the USART. */
    USART1->CTLR1 = USART_CTLR1_TE | USART_CTLR1_UE;
}

static void uart_write(const char *s) {
    while (*s) {
        while (!(USART1->STATR & USART_FLAG_TXE)) { /* spin */
        }
        USART1->DATAR = (uint16_t)(*s++);
    }
}

int main(void) {
    SystemInit();
    uart_init(BOARD_UART_BAUD);

    /* String literal concatenation builds the greeting at compile time --
     * no snprintf, no .rodata copy beyond the literal itself. */
    static const char greeting[] = "Hello from " BOARD_NAME " on " BOARD_MCU "\r\n";

    while (1) {
        uart_write(greeting);
        Delay_Ms(1000);
    }
}
