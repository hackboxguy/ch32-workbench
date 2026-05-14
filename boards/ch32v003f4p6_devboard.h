#ifndef BOARD_CH32V003F4P6_DEVBOARD_H
#define BOARD_CH32V003F4P6_DEVBOARD_H

/*
 * CH32V003F4P6 Dev Board (~$1.50 AliExpress SOP-8 board)
 *
 *   MCU:        CH32V003F4P6 (TSSOP-20)
 *   LED:        PD6, active high (on-board user LED next to USB-C)
 *   Button:     PC1, active low (on-board user button with pull-up)
 *   UART TX:    PD5 @ 115200 8N1 (USART1 default mapping)
 *   Programmer: WCH-LinkE on the 4-pin header (3V3 / GND / SWIO=PD1 / NRST)
 *
 * Verified against typical AliExpress dev-board schematics. If your board
 * differs (some clone variants put the LED on PD4), edit BOARD_LED_* below.
 *
 * Contract reminder: this header expects ch32fun.h to be included by the
 * .c file BEFORE board.h pulls this in. See boards/board.h.
 */

/* Identification */
#define BOARD_NAME                  "CH32V003F4P6 Dev Board"
#define BOARD_MCU                   "CH32V003F4P6"

/* User LED */
#define BOARD_HAS_LED               1
#define BOARD_LED_PORT              GPIOD
#define BOARD_LED_PIN               6
#define BOARD_LED_RCC_BIT           RCC_APB2Periph_GPIOD
#define BOARD_LED_ACTIVE_HIGH       1

/* User button */
#define BOARD_HAS_BUTTON            1
#define BOARD_BUTTON_PORT           GPIOC
#define BOARD_BUTTON_PIN            1
#define BOARD_BUTTON_RCC_BIT        RCC_APB2Periph_GPIOC
#define BOARD_BUTTON_ACTIVE_LOW     1

/* Default UART (USART1 TX = PD5 on CH32V003) */
#define BOARD_UART_TX_PORT          GPIOD
#define BOARD_UART_TX_PIN           5
#define BOARD_UART_TX_RCC_BIT       RCC_APB2Periph_GPIOD
#define BOARD_UART_BAUD             115200

#endif /* BOARD_CH32V003F4P6_DEVBOARD_H */
