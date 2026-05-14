#ifndef BOARD_CH32V003A4M6_QFN20_H
#define BOARD_CH32V003A4M6_QFN20_H

/*
 * CH32V003A4M6 QFN-20 (more GPIOs than F4P6/J4M6)
 *
 *   MCU:        CH32V003A4M6 (QFN-20)
 *   LED:        PD6, active high  -- TODO(verify): user-wired
 *   Button:     PC1, active low   -- TODO(verify): user-wired
 *   UART TX:    PD5 @ 115200 8N1
 *   Programmer: WCH-LinkE on SWIO=PD1
 *
 * QFN-20 exposes more GPIOs than the SOP-8 (J4M6) and TSSOP-20 (F4P6)
 * packages -- the full PA0..PA2, PC0..PC7, PD0..PD7 set. The pin choices
 * here mirror the F4P6 devboard so apps tuned for that board move over
 * with no source edits.
 */

#define BOARD_NAME              "CH32V003A4M6 QFN-20"
#define BOARD_MCU               "CH32V003A4M6"

#define BOARD_HAS_LED           1
#define BOARD_LED_PORT          GPIOD
#define BOARD_LED_PIN           6
#define BOARD_LED_RCC_BIT       RCC_APB2Periph_GPIOD
#define BOARD_LED_ACTIVE_HIGH   1

#define BOARD_HAS_BUTTON        1
#define BOARD_BUTTON_PORT       GPIOC
#define BOARD_BUTTON_PIN        1
#define BOARD_BUTTON_RCC_BIT    RCC_APB2Periph_GPIOC
#define BOARD_BUTTON_ACTIVE_LOW 1

#define BOARD_UART_TX_PORT      GPIOD
#define BOARD_UART_TX_PIN       5
#define BOARD_UART_TX_RCC_BIT   RCC_APB2Periph_GPIOD
#define BOARD_UART_BAUD         115200

#endif /* BOARD_CH32V003A4M6_QFN20_H */
