#ifndef BOARD_CH32V003J4M6_MINIMAL_H
#define BOARD_CH32V003J4M6_MINIMAL_H

/*
 * CH32V003J4M6 minimal board (bare SOP-8 chip on perfboard)
 *
 *   MCU:        CH32V003J4M6 (SOP-8, 5 GPIOs pinned out)
 *   LED:        PC1, active high  -- TODO(verify): user-wired
 *   Button:     PD4, active low (with external pull-up) -- TODO(verify): user-wired
 *   UART TX:    PD5 @ 115200 8N1 -- TODO(verify): user-wired
 *   Programmer: WCH-LinkE on SWIO=PD1
 *
 * SOP-8 pin map (typical):
 *   1: PD6      5: PD1 (SWIO)
 *   2: PD5      6: PA1
 *   3: PD4      7: PA2
 *   4: VSS      8: VDD
 *
 * This is a "build your own" perfboard target. Pin choices assume you wire
 * the LED to PC1 and the button to PD4 -- but PC1 is NOT pinned out on
 * SOP-8 J4M6 in many datasheet revisions! If your build can't use PC1,
 * pick a free pin from the map above and edit BOARD_LED_PORT/PIN.
 */

#define BOARD_NAME              "CH32V003J4M6 minimal (perfboard)"
#define BOARD_MCU               "CH32V003J4M6"

#define BOARD_HAS_LED           1
#define BOARD_LED_PORT          GPIOC
#define BOARD_LED_PIN           1
#define BOARD_LED_RCC_BIT       RCC_APB2Periph_GPIOC
#define BOARD_LED_ACTIVE_HIGH   1

#define BOARD_HAS_BUTTON        1
#define BOARD_BUTTON_PORT       GPIOD
#define BOARD_BUTTON_PIN        4
#define BOARD_BUTTON_RCC_BIT    RCC_APB2Periph_GPIOD
#define BOARD_BUTTON_ACTIVE_LOW 1

#define BOARD_UART_TX_PORT      GPIOD
#define BOARD_UART_TX_PIN       5
#define BOARD_UART_TX_RCC_BIT   RCC_APB2Periph_GPIOD
#define BOARD_UART_BAUD         115200

#endif /* BOARD_CH32V003J4M6_MINIMAL_H */
