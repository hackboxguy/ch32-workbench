#ifndef BOARD_H
#define BOARD_H

/*
 * boards/board.h - umbrella header for board selection.
 *
 * INCLUDE-ORDER CONTRACT:
 *
 *   Apps MUST `#include "ch32fun.h"` BEFORE `#include "board.h"`, so that
 *   GPIOx, RCC_APB2Periph_GPIOx, and the related register macros are
 *   already in scope when the selected board header is processed.
 *
 *   Per-board headers (ch32v003*.h, etc.) only define BOARD_* macros that
 *   reference symbols ch32fun has already provided. They do NOT include
 *   ch32fun themselves.
 *
 *   .clang-format keeps SortIncludes off so tooling cannot re-order the
 *   ch32fun.h / board.h pair.
 *
 * The compiler is invoked with -DBOARD_<UPPERCASE_NAME>. The block below
 * selects the matching board header.
 */

#if   defined(BOARD_CH32V003F4P6_DEVBOARD)
  #include "ch32v003f4p6_devboard.h"
#elif defined(BOARD_CH32V003J4M6_MINIMAL)
  #include "ch32v003j4m6_minimal.h"
#elif defined(BOARD_CH32V003A4M6_QFN20)
  #include "ch32v003a4m6_qfn20.h"
#else
  #error "No BOARD_* defined. Pass BOARD=<name> to make."
#endif

#endif /* BOARD_H */
