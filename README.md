# ch32-workbench

A CLI-first scaffold for bare-metal C development on **WCH CH32V-series RISC-V microcontrollers** (primary target: **CH32V003**). Multiple apps, multiple board definitions, one tree, one-line install + build + flash. Built on top of [`cnlohr/ch32fun`](https://github.com/cnlohr/ch32fun) — credit there for the actual MCU heavy lifting.

## Quick start

On a fresh Ubuntu 22.04 / 24.04 / Debian 12 host (or WSL2):

```bash
git clone --recurse-submodules https://github.com/<user>/ch32-workbench
cd ch32-workbench
./scripts/setup.sh                          # apt deps + udev rules (sudo)
cd apps/blink
make                                        # builds for default board
make flash                                  # programs target via WCH-LinkE
```

Switching boards:

```bash
make BOARD=ch32v003j4m6_minimal
make flash BOARD=ch32v003j4m6_minimal
```

Building everything from the top:

```bash
make            # builds all apps for their default boards
make BOARD=<name>   # overrides every app's default
make clean
```

If any of the above takes more than the listed commands, that's a bug — please file an issue.

## Supported MCUs / boards

v1 ships boards for **CH32V003** (16 KB flash, 2 KB SRAM, single-core RISC-V32, ~48 MHz). The scaffold is forward-compatible with the larger V20x / V30x families — see [docs/ch32-workbench-setup-prd.md](docs/ch32-workbench-setup-prd.md) §17.

| Board header | MCU | Package | LED | Button | UART TX |
|---|---|---|---|---|---|
| [`ch32v003f4p6_devboard.h`](boards/ch32v003f4p6_devboard.h) | CH32V003F4P6 | TSSOP-20 | PD6 (active high) | PC1 (active low) | PD5 |
| [`ch32v003j4m6_minimal.h`](boards/ch32v003j4m6_minimal.h)   | CH32V003J4M6 | SOP-8    | PC1 (active high) | PD4 (active low) | PD5 |
| [`ch32v003a4m6_qfn20.h`](boards/ch32v003a4m6_qfn20.h)       | CH32V003A4M6 | QFN-20   | PD6 (active high) | PC1 (active low) | PD5 |

Default UART config across all boards: USART1 @ 115200 8N1.

## Adding a new app

```bash
cp -r apps/blink apps/my_app
# edit apps/my_app/Makefile -- change TARGET := my_app
# write your code in apps/my_app/my_app.c
# include "ch32fun.h" first, then "board.h" -- never the other way around
cd apps/my_app && make
```

The per-app Makefile only sets `TARGET`, `TARGET_MCU`, `DEFAULT_BOARD`, the `CH32FUN` submodule path, and `EXTRA_CFLAGS`. Everything else comes from `$(CH32FUN)/ch32fun.mk`. **Use `EXTRA_CFLAGS`, not `CFLAGS`** — the latter preempts ch32fun.mk's optimization defaults and silently kills `-Os -flto -ffunction-sections`.

## Adding a new board

1. Create `boards/<name>.h` (lowercase, underscores). Header guard: `BOARD_<UPPERCASE_NAME>_H`.
2. Add a leading comment block documenting the wiring.
3. Define every macro from [boards/README.md](boards/README.md) ("Required macros").
4. Add a matching `#elif defined(BOARD_<UPPERCASE_NAME>)` arm to [boards/board.h](boards/board.h).
5. Verify: `cd apps/blink && make BOARD=<name>`.

If you can't physically verify a pin choice, leave a `TODO(verify):` comment rather than guessing silently.

## Toolchain versions

- **`gcc-riscv64-unknown-elf`**: tested with 13.2.0 (Ubuntu 24.04's `apt show gcc-riscv64-unknown-elf`).
- **ch32fun submodule**: pinned at commit [`c29e297`](https://github.com/cnlohr/ch32fun/commit/c29e297a72dc3e4d2e2e8d6624e1b75536822807) (`v1.0rc2-1014-gc29e297`). To bump: `cd ch32fun && git checkout <new-sha> && cd .. && git add ch32fun && git commit`.
- **make**: GNU make 4.x (any recent version).
- **`minichlink`**: built on demand from the submodule when you first `make flash`. Requires `libusb-1.0-0-dev` (covered by `setup.sh`).

## Programmer setup

Wire your WCH-LinkE to the target:

| WCH-LinkE | Target |
|---|---|
| `3V3`   | `3V3` |
| `GND`   | `GND` |
| `SWIO`  | `PD1` |
| (no NRST needed for CH32V003) | |

Then: `cd apps/blink && make flash`. The first flash builds `minichlink` from the submodule (one-time, ~5 s).

**Alternative (no WCH-LinkE)**: `minichlink` supports using a second CH32V003 as a programmer over its single-wire SDI interface, and can also flash through the CH32V003's factory USB DFU bootloader (hold BOOT0 low while plugging in USB). See `ch32fun/minichlink/README.md` for both.

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

## Troubleshooting (top 6)

1. **Chip not detected.** Check 3V3 / GND / SWIO wiring. The WCH-LinkE has a button — press it to switch between WCH-Link mode and DAP-Link mode (we want the former). If you've been programming a different MCU family recently, the programmer may be in the wrong mode.
2. **`Permission denied` on `/dev/hidraw*`.** Either udev rules aren't loaded (run `./scripts/setup.sh` again), or you need to log out and back in so the `uaccess` tag picks up. On WSL2, see "Programmer setup / WSL2" above — udev alone is not enough.
3. **Wrong board selected → blink/UART silent.** Confirm `BOARD=...` matches the physical board. Default is `ch32v003f4p6_devboard`. Run `make list-boards` to see what's available.
4. **Stale submodule.** After `git pull` updates the pinned ch32fun ref, run `git submodule update --init --recursive`.
5. **BOOT0 stuck (chip re-enters ISP every reset).** The CH32V003 enters factory USB DFU when BOOT0 is pulled high at reset. If your board ties BOOT0 high permanently, the chip never runs your code. Pull BOOT0 low at idle; only force it high when you intentionally want ISP.
6. **First `make flash` fails on libusb headers.** You skipped `./scripts/setup.sh`. The `minichlink` build needs `libusb-1.0-0-dev` and `libudev-dev`.

## License & credits

[MIT](LICENSE), © 2026 ch32-workbench contributors.

Built on top of **[cnlohr/ch32fun](https://github.com/cnlohr/ch32fun)** — the bare-metal substrate and `minichlink` programmer that make this whole thing tractable. If this scaffold is useful to you, ch32fun is the upstream that earned that praise.
