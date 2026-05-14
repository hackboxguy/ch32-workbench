#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

/*
 * uart_hello drives USART1 directly via registers (polled, no printf).
 * Disable ch32fun's own UART printf path so we don't pull it in.
 */

#define FUNCONF_USE_DEBUGPRINTF 0
#define FUNCONF_USE_UARTPRINTF  0

#endif
