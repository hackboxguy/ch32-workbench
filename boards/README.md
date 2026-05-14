# Boards

Board headers describe the physical wiring of a target board as a set of
`BOARD_*` macros that apps consume. The same app source compiles unchanged
for any board that supports its required peripherals.

## How board selection works

1. The user passes `BOARD=<name>` to `make` (or accepts the per-app
   `DEFAULT_BOARD`).
2. The per-app Makefile turns that into `-DBOARD_<UPPERCASE_NAME>` at
   compile time.
3. `boards/board.h` is the umbrella header: it tests the `BOARD_*` macro
   and `#include`s the matching board header.
4. The board header defines `BOARD_LED_*`, `BOARD_BUTTON_*`,
   `BOARD_UART_*`, etc. Apps reference only these symbols, never raw
   port/pin numbers.

## Include-order contract

Apps MUST include `ch32fun.h` before `board.h`, so that `GPIOx`,
`RCC_APB2Periph_GPIOx`, and friends are already in scope. Board headers
do NOT include ch32fun themselves; they assume its symbols are visible.

`.clang-format` keeps `SortIncludes: false` so tooling cannot break this
ordering.

## Required macros (minimum)

Every board header must define:

| Macro | Purpose |
|---|---|
| `BOARD_NAME` | Human-readable string, e.g. `"CH32V003F4P6 Dev Board"` |
| `BOARD_MCU` | MCU part number string, e.g. `"CH32V003F4P6"` |
| `BOARD_HAS_LED` | `1` if a user LED exists |
| `BOARD_LED_PORT` / `BOARD_LED_PIN` | GPIO port + pin number |
| `BOARD_LED_RCC_BIT` | The `RCC_APB2Periph_GPIOx` bit to enable the port clock |
| `BOARD_LED_ACTIVE_HIGH` | `1` for active high, `0` for active low |
| `BOARD_HAS_BUTTON` | `1` if a user button exists |
| `BOARD_BUTTON_PORT` / `BOARD_BUTTON_PIN` / `BOARD_BUTTON_RCC_BIT` | as above |
| `BOARD_BUTTON_ACTIVE_LOW` | `1` for active low (most buttons with pull-up) |
| `BOARD_UART_TX_PORT` / `BOARD_UART_TX_PIN` / `BOARD_UART_TX_RCC_BIT` | UART TX pin |
| `BOARD_UART_BAUD` | Default baud rate |

Apps that need a feature the board doesn't have should `#error` at
compile time on the matching `BOARD_HAS_*` macro.

## How to add a new board

1. Create `boards/<name>.h` (lowercase, with underscores). Header guard:
   `BOARD_<UPPERCASE_NAME>_H`.
2. Add a leading comment block documenting the physical wiring assumed.
3. Define all the required macros above.
4. Add a matching `#elif defined(BOARD_<UPPERCASE_NAME>)` arm to
   `boards/board.h`.
5. Verify by running `cd apps/blink && make BOARD=<name>`.

If you cannot verify a pin choice against hardware, leave a clear
`TODO(verify):` comment rather than guessing silently.

## Boards shipped in v1

| Header | MCU | Notes |
|---|---|---|
| `ch32v003f4p6_devboard.h` | CH32V003F4P6 (TSSOP-20) | The common ~$1.50 AliExpress dev board. Default for all v1 apps. |
| `ch32v003j4m6_minimal.h` | CH32V003J4M6 (SOP-8) | Bare chip on perfboard. Pin choices marked `TODO(verify):` -- LED on PC1 may not be pinned out on every J4M6 revision. |
| `ch32v003a4m6_qfn20.h` | CH32V003A4M6 (QFN-20) | More GPIOs available; pin choices mirror the F4P6 devboard. `TODO(verify):` against your actual wiring. |
