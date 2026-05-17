# ch32-workbench

A CLI-first scaffold for bare-metal C development on **WCH CH32V-series RISC-V microcontrollers** — primary target the **CH32V003** (a ~$0.10 32-bit MCU with 16 KB flash and 2 KB SRAM). Multiple apps, multiple board definitions, all in one tree. One-line install, one-line build, one-line flash.

Built on top of [`cnlohr/ch32fun`](https://github.com/cnlohr/ch32fun) — that's where the real MCU heavy lifting lives.

**Who this is for:** anyone who wants to mess with cheap RISC-V microcontrollers from a terminal without spending an afternoon fighting toolchains, IDEs, or vendor SDKs. If you can use `make` and `git`, you have everything you need.

## Quick start

The fastest "this is real" demo uses an SSD1306 OLED — it runs a graphics intro and then ticks a clock at 1 Hz, so you get instant visual confirmation that everything works.

What you'll need:

- A CH32V003 dev board (the ~$2 USB-C boards on AliExpress with a CH32V003F4P6 work fine)
- A **WCH-LinkE** programmer (~$3–5, often bundled with the dev board)
- An SSD1306 128×64 I2C OLED module (~$3) plus 4 jumper wires
- A Linux host (Ubuntu 22.04 / 24.04 / Debian 12, or WSL2 on Windows)

Wire it up:

| WCH-LinkE | CH32V003 board | Why |
|---|---|---|
| `3V3` | `3V3` | power |
| `GND` | `GND` | ground |
| `SWIO` | `PD1` | programming / debug |

| OLED | CH32V003 board |
|---|---|
| `VCC` | `3V3` |
| `GND` | `GND` |
| `SCL` | `PC2` |
| `SDA` | `PC1` |

Then on the host:

```bash
git clone --recurse-submodules https://github.com/<user>/ch32-workbench
cd ch32-workbench
./scripts/setup.sh                          # apt deps + udev rules (sudo)
cd apps/i2c_ssd1306_oled_test
make flash
```

You should see a ~18-second graphics intro (binary fill → pixels → lines → circles → bomb image → text in four sizes), then `HH:MM:SS` counting up from `00:00:00`. Total time from a clean checkout to "it works": under 2 minutes if the toolchain is already installed.

**Don't have an OLED?** The classic `blink` is there too:

```bash
cd apps/blink && make flash
```

…but note that not every "CH32V003 dev board" actually has a controllable user LED. If you see no blink, that doesn't necessarily mean the chip isn't running — see [Diagnostics](#diagnostics) below.

## Apps shipped

| App | Purpose | Hardware needed |
|---|---|---|
| [`blink`](apps/blink) | Toggle the user LED at 2 Hz using `BOARD_LED_*` macros. The "hello world" of MCU work. | A board with a controllable user LED. |
| [`uart_hello`](apps/uart_hello) | Polled USART1 transmit. Sends `"Hello from <BOARD_NAME> on <BOARD_MCU>\r\n"` once per second. | A serial monitor on the board's USART1 TX (PD5 on CH32V003). |
| [`gpio_button`](apps/gpio_button) | Mirror a user button onto the user LED with software debounce. | LED + button. Compile-errors on boards without a button. |
| [`i2c_ssd1306_oled_test`](apps/i2c_ssd1306_oled_test) | 9-mode graphics demo intro, then a running `HH:MM:SS` clock at 1 Hz. | SSD1306 128×64 I2C OLED on PC1/PC2. |
| [`thermal_regulator`](apps/thermal_regulator) | Closed-loop PWM fan controller — a DS18B20 sets a Noctua NF-A8's duty cycle; temperature, duty, and tach speed show on the OLED and stream once/sec on a 115200 serial line. | DS18B20, 4-pin PWM fan, 12 V supply, SSD1306 OLED. See "Hardware setup" below. |
| [`pin_sweep`](apps/pin_sweep) | **Diagnostic.** Drives every CH32V003 GPIO in sequence, ~2 s per pin, with UART announcements. | Used to find which pin actually drives the LED on an unknown board. |
| [`blink_legacy`](apps/blink_legacy) | **Diagnostic.** Drives PD0+PD4+PD6+PC0 simultaneously at 2 Hz. | A sanity check: if even this doesn't visibly blink anything, your board likely has no GPIO-controlled user LED. |

### Hardware setup: `thermal_regulator`

This app needs more parts than the others. Bill of materials:

- CH32V003 board + WCH-LinkE
- SSD1306 128×64 I2C OLED
- DS18B20 temperature sensor
- A 4-pin PWM fan (developed against a Noctua NF-A8)
- A 12 V supply for the fan
- Resistors: 4.7 kΩ (1-Wire pull-up), 10 kΩ (tach pull-up)
- Capacitor: ~10 nF (tach noise filter — **required**, see below)

Wiring:

| CH32V003 | Connects to |
|---|---|
| PC1 / PC2 | OLED SDA / SCL |
| PD4 | fan PWM input (pin 4, blue) |
| PD0 | fan tach (pin 3, green) |
| PD3 | DS18B20 DQ |
| PC0 | host serial out — USART1 TX, 115200 8N1 |
| VDD | OLED VCC, DS18B20 VDD, all three pull-ups |
| GND | OLED GND, DS18B20 GND, fan GND (pin 1), tach cap, **12 V PSU negative** |

Fan +12 V (pin 2, yellow) → the 12 V supply's positive terminal. The 1-Wire pull-up is 4.7 kΩ from PD3 to VDD; the tach pull-up is 10 kΩ from PD0 to VDD; the tach cap is ~10 nF from PD0 to GND.

**Host serial:** PC0 streams a once-per-second status line (`T=… duty=… tach=… rpm=…`) at 115200 8N1. Wire PC0 to a USB-serial adapter's RX (common ground), or to the WCH-LinkE's RX header pin to read it on the WCH-LinkE's USB-CDC port (`/dev/ttyACM0`). The same line also goes to the SWIO debug channel — view that with `minichlink -T`. USART1's default TX is PD5; it's remapped to PC0 here because PD5 isn't broken out on the V1772 board and PD0 is taken by the tach.

Pull-ups go to **VDD** — 3.3 V or 5 V depending on your board. (The V1772 board has no regulator and runs the CH32V003 at 5 V, so pull up to 5 V there.) Never pull a pin above VDD.

Two gotchas, both learned the hard way on the bench:

- **Common ground is non-negotiable.** The 12 V PSU negative must tie directly to the board GND — short, solid, one point. Ground bounce otherwise corrupts the tach reading.
- **The ~10 nF tach cap is required, not optional.** A 4-pin fan's motor commutation dumps EMI onto the tach line; without the cap the tach reads pinned near maximum regardless of real speed. With it (a ~1.6 kHz low-pass against the 10 kΩ pull-up) the reading tracks correctly. 10–22 nF works; don't exceed ~100 nF.

Fan curve, hysteresis, and pin assignments are tunable `#define`s at the top of [`thermal_regulator.c`](apps/thermal_regulator/thermal_regulator.c).

## Switching boards

Each app's Makefile picks a `DEFAULT_BOARD`. Override it on the command line:

```bash
cd apps/blink
make BOARD=ch32v003j4m6_minimal
make flash BOARD=ch32v003j4m6_minimal
```

From the repo root, override every app at once:

```bash
make                       # all apps for their defaults
make BOARD=<name>          # all apps for one named board
make clean
make list-apps             # discover apps
make list-boards           # discover boards
```

## Supported MCUs / boards

v1 targets **CH32V003** (16 KB flash, 2 KB SRAM, RV32EC, ~48 MHz HSI). The scaffold is forward-compatible with larger CH32V parts (V20x, V30x) — see [docs/ch32-workbench-setup-prd.md](docs/ch32-workbench-setup-prd.md) §17 for the FreeRTOS roadmap.

| Board header | MCU | Package | LED | Button | UART TX |
|---|---|---|---|---|---|
| [`ch32v003f4p6_devboard.h`](boards/ch32v003f4p6_devboard.h) | CH32V003F4P6 | TSSOP-20 | PD6 (active high) | PC1 (active low) | PD5 |
| [`ch32v003j4m6_minimal.h`](boards/ch32v003j4m6_minimal.h)   | CH32V003J4M6 | SOP-8    | PC1 (active high) | PD4 (active low) | PD5 |
| [`ch32v003a4m6_qfn20.h`](boards/ch32v003a4m6_qfn20.h)       | CH32V003A4M6 | QFN-20   | PD6 (active high) | PC1 (active low) | PD5 |

The LED/button pin assignments above are the *expected* wiring for the "common" variant of each board. Real-world clones vary; if your specific board doesn't behave, see [Diagnostics](#diagnostics). USART1 @ 115200 8N1 on every board.

## Adding a new app

```bash
cp -r apps/blink apps/my_app
# edit apps/my_app/Makefile -- change TARGET := my_app
# rename apps/my_app/blink.c -> my_app.c, edit to taste
# always #include "ch32fun.h" BEFORE "board.h" -- never the other way around
cd apps/my_app && make
```

The per-app Makefile only sets `TARGET`, `TARGET_MCU`, `DEFAULT_BOARD`, the `CH32FUN` submodule path, and `EXTRA_CFLAGS`. Everything else comes from `$(CH32FUN)/ch32fun.mk`. **Use `EXTRA_CFLAGS`, not `CFLAGS`** — the latter preempts ch32fun.mk's optimization defaults and silently kills `-Os -flto -ffunction-sections`. (This bit us during scaffold bring-up; the symptom was a 14× larger binary that linked but didn't run for non-trivial apps.)

## Adding a new board

1. Create `boards/<name>.h` (lowercase, underscores). Header guard: `BOARD_<UPPERCASE_NAME>_H`.
2. Add a leading comment block documenting the physical wiring assumed.
3. Define every macro from [boards/README.md](boards/README.md) ("Required macros").
4. Add a matching `#elif defined(BOARD_<UPPERCASE_NAME>)` arm to [boards/board.h](boards/board.h).
5. Verify: `cd apps/blink && make BOARD=<name>` builds cleanly.

If you can't physically verify a pin choice, leave a `TODO(verify):` comment in the header rather than guessing silently.

## Diagnostics

When `blink` doesn't visibly blink and you're not sure whether it's the code, the chip, the programmer, or the board, work through these in order:

1. **`make flash` output looks healthy.** It should say `Detected CH32V003`, print a Part UUID, and end with `Image written.`. If it doesn't, you have a programmer/wiring problem, not a code problem.
2. **`apps/blink_legacy`** drives **PD0+PD4+PD6+PC0 simultaneously** at 2 Hz. These are the most common user-LED pins across CH32V003 dev-board variants. If at least one of them lights an LED, the chip is running and you can move on to step 3 to find the exact pin. If none of them do, your board likely has no GPIO-controlled user LED at all (some clones only have a hardwired power LED).
3. **`apps/pin_sweep`** drives **every accessible GPIO** in sequence, ~2 s per pin, with `PIN: <name>` announcements on USART1 (PD5 @ 115200). Watch for which 2-second burst lights your LED; that's your pin. If you have a UART monitor on the WCH-LinkE's USB-CDC, you can read the pin name directly.
4. **Still nothing.** Probe a known-toggling pin with a scope or LED+resistor — `blink_legacy` puts square waves on PD0/PD4/PD6/PC0 every 250 ms. If they're toggling, the chip is fine and the issue is purely "no LED wired anywhere we can drive it."

`apps/pin_sweep` skips PD1 (it's SWIO, the programming pin — driving it during a sweep would fight the programmer). PD5 also doubles as USART1 TX, so the UART log briefly stops during that pin's burst.

## Toolchain versions

- **`gcc-riscv64-unknown-elf`**: tested with 13.2.0 (Ubuntu 24.04's `apt show gcc-riscv64-unknown-elf`). 9.x on older Ubuntu also works.
- **ch32fun submodule**: pinned at commit [`c29e297`](https://github.com/cnlohr/ch32fun/commit/c29e297a72dc3e4d2e2e8d6624e1b75536822807) (`v1.0rc2-1014-gc29e297`). To bump: `cd ch32fun && git checkout <new-sha> && cd .. && git add ch32fun && git commit`.
- **make**: GNU make 4.x (any recent version).
- **`minichlink`**: built on demand from the submodule the first time you `make flash`. Requires `libusb-1.0-0-dev` (covered by `setup.sh`).
- **clang-format**: only needed for the lint pass in CI; 14+ works.

## Programmer setup

Wire your WCH-LinkE to the target:

| WCH-LinkE | CH32V003 |
|---|---|
| `3V3`   | `3V3` |
| `GND`   | `GND` |
| `SWIO`  | `PD1` |

NRST is not required for CH32V003. The first `make flash` will build `minichlink` from the submodule (one-time, ~5 s).

**WCH-Link vs WCH-LinkE**: there are two products. The newer **WCH-LinkE** (with an "E") works reliably. The older "WCH-Link" can have firmware quirks with CH32V003 — if you have one, the button on the side toggles between WCH-Link and DAP-Link modes; we want WCH-Link mode.

**Alternative (no WCH-LinkE)**: `minichlink` can also use a second CH32V003 as a programmer over its single-wire SDI interface, or flash through the chip's factory USB DFU bootloader (hold BOOT0 low while plugging in USB). See `ch32fun/minichlink/README.md`.

### WSL2

USB access to the WCH-LinkE is not automatic on WSL2. From the **Windows** host (PowerShell as admin, once-off):

```powershell
winget install --interactive --exact dorssel.usbipd-win
```

Then on every reboot or replug:

```powershell
usbipd list                          # find the WCH-LinkE bus-id
usbipd bind --busid <id>             # one-time (persists)
usbipd attach --wsl --busid <id>     # attach to the running WSL2 distro
```

After attach, the device appears as `/dev/hidraw*` inside WSL2 and the udev rules from `setup.sh` grant your user access.

## Repo layout

```
ch32-workbench/
├── README.md                this file
├── LICENSE                  MIT
├── Makefile                 top-level: iterates apps/*, propagates BOARD=
├── ch32fun/                 git submodule -> github.com/cnlohr/ch32fun
├── boards/                  per-board headers; pick with BOARD=<name>
├── apps/                    one directory per app, each is independent
│   ├── blink/
│   ├── uart_hello/
│   ├── gpio_button/
│   ├── i2c_ssd1306_oled_test/
│   ├── pin_sweep/           diagnostic
│   └── blink_legacy/        diagnostic
├── common/                  shared code (empty in v1, your future helpers go here)
├── scripts/                 setup.sh + udev rules
├── ci/                      helper scripts for GitHub Actions
└── docs/                    PRD + implementation plan
```

## Troubleshooting

1. **Chip not detected.** Check 3V3 / GND / SWIO wiring. The WCH-LinkE button toggles between WCH-Link and DAP-Link mode — we want WCH-Link. If you've been programming an ARM target recently, the programmer may be in the wrong mode.
2. **`Permission denied` on `/dev/hidraw*`.** Either udev rules aren't loaded (re-run `./scripts/setup.sh`), or you need to log out and back in so the `uaccess` tag takes effect. On WSL2, see the WSL2 subsection above — udev alone is not enough.
3. **`blink` flashes successfully but no LED visibly blinks.** Your board may not have a GPIO-controlled user LED — many CH32V003 clones have only a hardwired power LED. Run `apps/blink_legacy` to test the four common LED pins at once; if that's also dark, run `apps/pin_sweep` to scan every GPIO. See [Diagnostics](#diagnostics).
4. **Wrong board selected → silent.** Confirm `BOARD=...` matches your physical board. `make list-boards` shows the available names. Default is `ch32v003f4p6_devboard`.
5. **Stale submodule.** After `git pull` updates the pinned ch32fun ref, run `git submodule update --init --recursive`.
6. **BOOT0 stuck (chip re-enters ISP every reset).** When BOOT0 is held high at reset, the CH32V003 boots into the factory USB DFU instead of running your flash. Verify with `lsusb | grep 4348` after a power cycle — if you see `4348:55e0`, BOOT0 is stuck high. Pull it low.
7. **First `make flash` fails on libusb headers.** You skipped `./scripts/setup.sh`. The `minichlink` build needs `libusb-1.0-0-dev` and `libudev-dev`.

## License & credits

[MIT](LICENSE), © 2026 ch32-workbench contributors.

Built on top of **[cnlohr/ch32fun](https://github.com/cnlohr/ch32fun)** — the bare-metal substrate and `minichlink` programmer that make this whole thing tractable. The SSD1306 driver in `ch32fun/extralibs/` (used by `i2c_ssd1306_oled_test`) is by E. Brombaugh. If this scaffold is useful to you, those are the upstream projects that earned the credit.
