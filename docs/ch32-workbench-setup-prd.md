# PRD: `ch32-workbench` — Multi-Board, Multi-App Development Scaffold for WCH CH32V RISC-V MCUs

**Version:** 1.0
**Scope:** v1 — bare-metal foundation, CH32V003-focused, with explicit forward-compatibility for CH32V203+ and an optional FreeRTOS variant (§17.1).

## 1. Purpose

Establish a minimal, CLI-first git repository for bare-metal C development on WCH CH32V-series RISC-V microcontrollers (primary target: **CH32V003F4P6**, with headroom to extend to other CH32V parts). The repo must support **multiple apps** built against **multiple board definitions** from a single tree, with a one-line install + one-line build + one-line flash workflow.

This PRD is the input to a fresh Claude Code session. The agent should produce the complete repo skeleton, all Makefiles, the board header convention, a setup script, a README, and three working sample apps.

**Naming.** The repo is named `ch32-workbench` rather than `ch32-baremetal` or `ch32v003-*` so the same tree can absorb future CH32V203/V307 boards and the optional FreeRTOS variant (§17.1) without the name becoming inaccurate. v1 is bare-metal, but the scaffold should not encode that as a permanent identity.

## 2. Non-Goals

- No IDE configuration (no VS Code `.vscode/`, no CLion, no Eclipse).
- No PlatformIO integration. No Arduino layer.
- No CMake (Make only — ch32fun is Make-based and we follow upstream).
- No Docker/container build environment (host toolchain via apt).
- No HAL. Bare metal in v1, talking directly to ch32fun's register-level API.
- No RTOS in v1 (FreeRTOS is reserved for the v2 variant described in §17.1 — the v1 scaffold must not preclude it but must not implement it).
- No deeper static analysis (cppcheck, clang-tidy, `gcc -fanalyzer`), no hardware-in-the-loop tests, no host-side unit tests. v1 ships a *minimal* CI defined in §15 (build matrix + size report + shellcheck + clang-format); anything beyond that is deferred.

## 3. Target User Workflow

A new user landing on this repo, on a fresh Ubuntu 22.04 / 24.04 / Debian 12 / WSL2 host, must be able to:

```bash
git clone --recurse-submodules https://github.com/<user>/ch32-workbench
cd ch32-workbench
./scripts/setup.sh                          # installs apt deps + udev rules
cd apps/blink
make                                        # builds for default board
make flash                                  # programs target via WCH-LinkE
```

And to switch boards:

```bash
make BOARD=ch32v003j4m6_minimal             # rebuilds for a different board
make flash BOARD=ch32v003j4m6_minimal
```

And to build everything from the top:

```bash
make            # builds all apps for their default boards
make clean
```

If any step requires more than what's listed above, the PRD has not been met.

## 4. Repository Layout (exact)

```
ch32-workbench/
├── README.md
├── LICENSE                          # MIT (matches ch32fun upstream)
├── Makefile                         # top-level: builds all apps
├── .gitignore
├── .gitmodules
├── .clang-format                    # repo-wide formatting style (enforced by CI)
├── .github/
│   └── workflows/
│       └── ci.yml                   # minimal CI: build matrix + size + shellcheck + clang-format
├── ch32fun/                         # git submodule → github.com/cnlohr/ch32fun
├── boards/
│   ├── README.md                    # how to add a new board
│   ├── board.h                      # umbrella header: #includes the BOARD-selected file
│   ├── ch32v003f4p6_devboard.h      # default board (cheap SOP-8 dev board)
│   ├── ch32v003j4m6_minimal.h       # bare SOP-8 chip on perfboard
│   └── ch32v003a4m6_qfn20.h         # QFN20 variant with more GPIOs
├── apps/
│   ├── blink/
│   │   ├── Makefile
│   │   └── blink.c
│   ├── uart_hello/
│   │   ├── Makefile
│   │   └── uart_hello.c
│   └── gpio_button/
│       ├── Makefile
│       └── gpio_button.c
├── common/                          # shared user code (optional, empty in v1)
│   └── .gitkeep
├── ci/                              # helper scripts invoked by CI (and runnable locally)
│   ├── README.md
│   └── size-report.sh               # riscv64-unknown-elf-size → markdown table
└── scripts/
    ├── setup.sh                     # apt-get deps + udev rules install
    └── 60-wch-link.rules            # udev rules for WCH-LinkE & SDI bootloader
```

## 5. Submodule

- `ch32fun/` shall be added as a git submodule pointing to **`https://github.com/cnlohr/ch32fun.git`**.
- Pin to a specific commit (whatever is current `master` at repo creation time) so future clones reproduce the build. Record the commit SHA in the README under "Toolchain versions".
- Do **not** vendor (copy in) ch32fun files. Keep it as a clean submodule.

## 6. Top-Level `Makefile`

Behavior:

- `make` (or `make all`) — iterate `apps/*/` and run `$(MAKE) -C` in each. Each app builds for its own `DEFAULT_BOARD` declared in its Makefile.
- `make clean` — iterate `apps/*/` and run `$(MAKE) -C $$d clean`.
- `make list-apps` — print discovered apps.
- `make list-boards` — print discovered board headers from `boards/*.h` (excluding `board.h`).
- `make help` — print available targets and usage.

Do not duplicate compiler flags here. Each app's Makefile owns its build via `ch32fun.mk`.

## 7. Per-App `Makefile` Template

Each app's Makefile is intentionally tiny. Template (use this exact shape for all three sample apps):

```make
# apps/<name>/Makefile
TARGET        := <name>
TARGET_MCU    := CH32V003
DEFAULT_BOARD ?= ch32v003f4p6_devboard
BOARD         ?= $(DEFAULT_BOARD)

# Path to ch32fun submodule
CH32FUN := ../../ch32fun/ch32fun

# Pass the board selection to the compiler so boards/board.h can pick the right include.
# -Wextra is added explicitly so the README's stated warning level holds regardless of
# whether the pinned ch32fun.mk enables it.
CFLAGS  += -I../../boards -DBOARD_$(shell echo $(BOARD) | tr a-z A-Z) -Wextra

ADDITIONAL_C_FILES :=

include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean
```

The agent must verify the actual path to `ch32fun.mk` inside the upstream submodule (it may live at `ch32fun/ch32fun.mk` or `ch32v003fun/ch32v003fun.mk` depending on upstream layout) and adjust the include path accordingly. Use the layout present in the pinned commit.

## 8. Board Abstraction

The goal: one app's `.c` source compiles unchanged for any board that supports its required peripherals. Boards are described as plain C headers.

### 8.1 `boards/board.h` (the umbrella)

**Include order contract:** apps MUST `#include "ch32fun.h"` *before* `#include "board.h"`, so that `GPIOx`, `RCC_APB2Periph_GPIOx`, and the related register macros are already in scope when the board header is processed. Per-board headers (`ch32v003*.h`) do NOT include ch32fun themselves; they only define `BOARD_*` macros that reference symbols ch32fun has already provided. `boards/board.h` documents this contract in a leading comment block.

```c
#ifndef BOARD_H
#define BOARD_H

/* The compiler is invoked with -DBOARD_<UPPERCASE_NAME>. Select the matching header. */
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
```

### 8.2 Per-board header convention

Every board header must define **at minimum**:

```c
/* Identification */
#define BOARD_NAME                  "CH32V003F4P6 Dev Board"
#define BOARD_MCU                   "CH32V003F4P6"

/* User LED (active level + GPIO port/pin) */
#define BOARD_HAS_LED               1
#define BOARD_LED_PORT              GPIOD
#define BOARD_LED_PIN               6
#define BOARD_LED_RCC_BIT           RCC_APB2Periph_GPIOD
#define BOARD_LED_ACTIVE_HIGH       1

/* User button (optional; BOARD_HAS_BUTTON 0 means absent) */
#define BOARD_HAS_BUTTON            1
#define BOARD_BUTTON_PORT           GPIOC
#define BOARD_BUTTON_PIN            1
#define BOARD_BUTTON_RCC_BIT        RCC_APB2Periph_GPIOC
#define BOARD_BUTTON_ACTIVE_LOW     1

/* Default UART for printf / hello world (USART1 on CH32V003) */
#define BOARD_UART_TX_PORT          GPIOD
#define BOARD_UART_TX_PIN           5
#define BOARD_UART_BAUD             115200
```

Apps reference these macros only, never raw GPIO numbers. If a board doesn't have a feature (e.g. `BOARD_HAS_BUTTON 0`), apps that require it must `#error` at compile time.

### 8.3 Initial board definitions to ship

| File | Description | LED pin assumption | Button pin assumption |
|---|---|---|---|
| `ch32v003f4p6_devboard.h` | The common ~$1.50 SOP-8 dev board sold on AliExpress (CH32V003F4P6 + USB-C + LED + button + WCH-Link header) | PD6, active high — agent should confirm against a typical board photo/schematic comment in the header | PC1, active low |
| `ch32v003j4m6_minimal.h` | Bare CH32V003**J**4M6 SOP-8 chip on perfboard | PC1, active high | PD4, active low (`TODO(verify):` user-wired) |
| `ch32v003a4m6_qfn20.h` | CH32V003A4M6 QFN-20 with extra GPIO available | PD6, active high | PC1, active low |

Pin choices in each header should be documented in a leading comment block. If the agent cannot verify a specific board's wiring, it should leave a clear `TODO(verify):` comment rather than guess silently.

## 9. Sample Apps to Ship (v1)

### 9.1 `apps/blink/blink.c`

- Uses `BOARD_LED_*` macros only.
- Initializes GPIO clock via `BOARD_LED_RCC_BIT`.
- Sets pin as push-pull output.
- Toggles LED at ~2 Hz using `Delay_Ms()` from ch32fun.
- Respects `BOARD_LED_ACTIVE_HIGH`.

### 9.2 `apps/uart_hello/uart_hello.c`

- Configures USART1 on `BOARD_UART_TX_*` at `BOARD_UART_BAUD`.
- Prints `"Hello from <BOARD_NAME> on <BOARD_MCU>\r\n"` once per second.
- Demonstrates how to send a string via USART without printf (keep flash tiny).

### 9.3 `apps/gpio_button/gpio_button.c`

- Compile-time guard: `#if !BOARD_HAS_BUTTON` → `#error`.
- Reads `BOARD_BUTTON_*` (pull-up, active-low or per board macro).
- Mirrors button state onto LED.
- Demonstrates simple debouncing in software (10–20 ms).

All three apps must build and link cleanly under `-Os -flto -Wall -Wextra` (these are set by `ch32fun.mk`; do not override).

## 10. `scripts/setup.sh`

POSIX-sh compatible (use `#!/bin/sh` or `#!/usr/bin/env bash` with strict mode). Behavior:

1. Detect distro via `/etc/os-release`; fail loudly on anything other than Ubuntu/Debian.
2. `sudo apt-get update`.
3. `sudo apt-get install -y` the following:
   - `gcc-riscv64-unknown-elf`
   - `make`
   - `build-essential`
   - `libusb-1.0-0-dev`
   - `libudev-dev`
   - `git`
   - `pkg-config`
4. Install `scripts/60-wch-link.rules` to `/etc/udev/rules.d/` (with sudo) and run `sudo udevadm control --reload-rules && sudo udevadm trigger`.
5. Print a final "Setup complete. Plug in WCH-LinkE and run `cd apps/blink && make flash` to verify." message.
6. Idempotent — running twice must be safe.

## 11. `scripts/60-wch-link.rules`

Provide udev rules to give non-root access to:

- WCH-LinkE programmer (USB VID `1a86`, PID `8010` and `8012`).
- CH32V003 in BOOT0 ISP mode (USB VID `4348`, PID `55e0`) — for users without a WCH-LinkE who flash via the factory USB bootloader.

Standard `MODE="0666"` is acceptable; preferring `TAG+="uaccess"` is also acceptable. Agent's choice, but pick one and stay consistent.

## 12. `.gitignore`

At minimum:

```
*.o
*.elf
*.bin
*.hex
*.lst
*.map
*.d
*.s
*.su
build/
compile_commands.json
.cache/
```

## 13. `README.md` Content Requirements

Must include, in order:

1. One-paragraph project description.
2. Quick start (the five commands from §3).
3. Supported MCUs / boards table.
4. How to add a new app (copy `apps/blink`, edit Makefile `TARGET`, write your `.c`).
5. How to add a new board (create `boards/<name>.h`, add `#elif defined(...)` to `board.h`).
6. Toolchain versions section, including:
   - `gcc-riscv64-unknown-elf` version (`apt show gcc-riscv64-unknown-elf` output expected on Ubuntu 24.04).
   - Pinned ch32fun submodule commit SHA.
7. Programmer setup (WCH-LinkE wiring: SWIO → PD1, GND → GND, 3V3 → 3V3; with a brief note that the alternative is `minichlink`'s "use another CH32V003 as a programmer" mode). Includes a short **WSL2 subsection** covering `usbipd-win` attach steps (`usbipd list`, `usbipd bind --busid <id>`, `usbipd attach --wsl --busid <id>`) since USB access to WCH-LinkE under WSL2 is not automatic.
8. Troubleshooting: top 6 likely issues (chip not detected; permission denied on /dev/hidraw; wrong board selected; stale submodule; BOOT0 stuck; first `make flash` fails on libusb headers → user skipped `setup.sh`).
9. License and credits (credit cnlohr/ch32fun prominently).

## 14. Coding Standards

- C11.
- Tabs vs spaces: 4 spaces, no tabs (in our code; ch32fun upstream is untouched).
- Header guards using `#ifndef BOARD_<NAME>_H` style.
- All board macros prefixed `BOARD_`.
- One blank line at EOF.
- No `#include <stdio.h>` in apps unless absolutely necessary — keep apps tiny.

## 15. CI / Quality Gates (Minimal)

A single GitHub Actions workflow at `.github/workflows/ci.yml` enforces the v1 quality bar. Anything beyond this tier (cppcheck, clang-tidy, `gcc -fanalyzer`, HIL) is deferred per §2.

### 15.1 Workflow shape

Two jobs in one workflow, triggered on `push` and `pull_request` against `main`:

**Job `build`** — matrix `os: [ubuntu-22.04, ubuntu-24.04]`:

1. `actions/checkout@v4` with `submodules: recursive`.
2. Install the build subset of `setup.sh`'s deps (no udev install in CI): `gcc-riscv64-unknown-elf`, `build-essential`, `libusb-1.0-0-dev`, `libudev-dev`, `pkg-config`.
3. Loop over `apps/*` × `boards/*.h` (excluding `boards/board.h`) and run `make -C apps/<app> BOARD=<board>` for each combination. All combinations must succeed.
4. `./ci/size-report.sh` collects `riscv64-unknown-elf-size` output for every built ELF (per app, per board) and writes a markdown table to `$GITHUB_STEP_SUMMARY`.
5. Upload built `.elf` files as a workflow artifact.

**Job `lint`** — single `ubuntu-24.04` runner:

1. `shellcheck scripts/setup.sh ci/*.sh` — fails on any warning.
2. `clang-format --dry-run --Werror` over every `.c` and `.h` in `apps/`, `boards/`, `common/` (NOT `ch32fun/`).

### 15.2 Repo-root `.clang-format`

Base style `LLVM` with these overrides: `IndentWidth: 4`, `UseTab: Never`, `ColumnLimit: 100`, `AllowShortFunctionsOnASingleLine: None`, `SortIncludes: false` (the include-order contract in §8.1 forbids reordering).

### 15.3 `ci/` helper scripts

`ci/size-report.sh` is the only script v1 requires. It runs locally as well as in CI, so contributors can sanity-check size before opening a PR. `ci/README.md` documents what each script does.

## 16. Acceptance Criteria

The repo is considered "done" for v1 when **all** of these pass on a fresh Ubuntu 24.04 VM (or WSL2):

1. `git clone --recurse-submodules <url>` succeeds and pulls ch32fun.
2. `./scripts/setup.sh` runs to completion with no manual intervention.
3. `cd apps/blink && make` produces `blink.elf`, `blink.bin`, and `blink.hex` without warnings.
4. `make BOARD=ch32v003j4m6_minimal` rebuilds for the alternate board (compiles successfully even if the user doesn't own that board).
5. `make flash` programs an attached CH32V003F4P6 dev board via WCH-LinkE and the LED begins blinking.
6. `cd apps/uart_hello && make flash` — a 115200 8N1 terminal on the board's UART TX pin shows the hello message at 1 Hz.
7. `cd apps/gpio_button && make flash` — pressing the user button toggles the LED.
8. Top-level `make` builds all three apps.
9. Top-level `make clean` removes all build artifacts.
10. `make list-apps` and `make list-boards` print the correct lists.
11. README's quick-start instructions, followed verbatim, work.
12. The GitHub Actions workflow described in §15 runs green on push: every app × every board builds on both Ubuntu 22.04 and 24.04, the size report appears in the step summary, and the lint job passes shellcheck and clang-format.

## 17. Out of Scope (Explicit) — to be revisited later

- Multi-MCU support beyond CH32V003 (CH32V203, CH32V307). The structure should not preclude this; `TARGET_MCU` is already per-app so adding a new family is mostly a board-header exercise plus verifying `ch32fun.mk` handles it.
- USB device stack (`rv003usb`) — separate future PRD.
- Power profiling / standby modes.
- Unit tests / host-side simulation.

### 17.1 FreeRTOS Variant (future, separate PRD)

ch32fun is a bare-metal substrate, not a FreeRTOS-first SDK like ESP-IDF. FreeRTOS is *not* included in v1, but the v1 scaffold must not preclude adding it later. When the FreeRTOS variant is implemented (separate PRD), it shall conform to the following constraints:

- **Opt-in per app.** A FreeRTOS app declares `USE_FREERTOS := 1` in its Makefile; bare-metal apps remain unchanged and unaffected. The default flow stays bare-metal.
- **Submodule layout.** FreeRTOS kernel sources live under `third_party/freertos-kernel/` as a separate git submodule pointing to `https://github.com/FreeRTOS/FreeRTOS-Kernel.git`, pinned to a known-good tag (>= V11.x). Do not vendor.
- **Per-app config.** Each FreeRTOS app owns its `FreeRTOSConfig.h` sitting alongside `main.c`. No global FreeRTOS config in the repo root.
- **Build glue.** A `mk/freertos.mk` snippet is included by the app Makefile when `USE_FREERTOS=1` and adds the kernel sources, the RISC-V port (`portable/GCC/RISC-V/`), the chosen heap implementation (default `heap_4.c`), and required include paths.
- **Reference port.** Integration follows the documented fixes from Matthew Tran's CH32V port (matthewtran.dev/2023/12/baremetal-c-cpp-on-ch32v):
  1. Disable interrupt nesting via the **INTSYSCR** CSR at startup (the QingKe core's `ecall` can preempt with interrupts disabled, causing memory corruption otherwise).
  2. Hook FreeRTOS's trap handler such that non-RTOS IRQs still dispatch through ch32fun's vector entries.
  3. Use a software interrupt for `portYIELD()` rather than `ecall` if INTSYSCR-based fix proves unreliable on a given chip variant.
- **Recommended targets.** FreeRTOS is *supported but discouraged* on CH32V003 (2 KB SRAM, 16 KB flash — realistic ceiling of 2–3 small tasks). It is *recommended* on **CH32V203** (20 KB SRAM, 64 KB flash) and larger CH32V parts, where it fits comfortably. The v2 PRD should make CH32V203 the primary FreeRTOS target.
- **Reference app.** Ship one demo: `apps/freertos_blinky/` with two tasks (LED blink + UART heartbeat) and a queue between them, parameterized by the same `BOARD_*` macros as bare-metal apps. Must build for both CH32V003 (as a tightness demo) and CH32V203 (the realistic target).
- **Non-FreeRTOS alternatives noted in README.** Before introducing FreeRTOS, the v2 PRD's README addendum shall briefly list lighter-weight options (super-loop + state machines, protothreads) and explain when each makes sense — so users don't reach for FreeRTOS reflexively on a 2 KB chip.

Implementation of this variant is explicitly **out of scope for v1**. The v1 agent shall not create `third_party/`, `mk/freertos.mk`, or any `USE_FREERTOS` plumbing. This subsection exists solely so the v1 layout doesn't paint v2 into a corner.

## 18. Deliverable

The agent shall:

1. Create every file listed in §4.
2. Initialize the ch32fun submodule and commit the pinned reference.
3. Make a single initial commit with message `Initial commit: ch32-workbench scaffold` (or a small series of logical commits if preferred — agent's call).
4. Print the SHA of the pinned ch32fun commit and a summary of files created.
5. Run `make` from the top level as a final sanity check and report the result.

If any acceptance criterion cannot be met without hardware (criteria 5, 6, 7 require a physical board), the agent shall note this and confirm that the *build* portion of the criterion passes.

---

**End of PRD.**
