#include "ch32fun.h"

/*
 * P2 stub: validates the toolchain + ch32fun + Makefile chain end-to-end.
 * Does NOT yet include "board.h" — that arrives in P3a when the per-board
 * headers exist. P3b replaces this stub with the real implementation that
 * uses BOARD_LED_* macros.
 */

int main(void)
{
    SystemInit();
    while (1) {
    }
    return 0;
}
