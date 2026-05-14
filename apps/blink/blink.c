#include "ch32fun.h"
#include "board.h"

/*
 * P3a stub: now also pulls in board.h to exercise the umbrella + per-board
 * headers across all three v1 boards. References BOARD_NAME so the macro
 * is actually consumed (the array forces the linker to keep it).
 * P3b replaces this with the real implementation.
 */

static const char board_name[] = BOARD_NAME;

int main(void)
{
    SystemInit();
    (void)board_name;
    while (1) {
    }
    return 0;
}
