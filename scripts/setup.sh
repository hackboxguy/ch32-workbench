#!/usr/bin/env bash
#
# ch32-workbench host setup -- installs apt build dependencies and the
# udev rules needed to talk to the WCH-LinkE programmer without sudo.
#
# Idempotent: run as often as you like.
# Targets: Ubuntu 22.04 / 24.04, Debian 12. Other distros: see the apt
# package list below and install the equivalents manually.

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
UDEV_RULE_SRC="${SCRIPT_DIR}/60-wch-link.rules"
UDEV_RULE_DST="/etc/udev/rules.d/60-wch-link.rules"

err()  { printf 'setup.sh: %s\n' "$*" >&2; }
info() { printf 'setup.sh: %s\n' "$*"; }

# -- 1. Distro check ---------------------------------------------------------

if [[ ! -r /etc/os-release ]]; then
    err "cannot read /etc/os-release; this script targets Ubuntu/Debian only"
    exit 1
fi
# shellcheck disable=SC1091
. /etc/os-release
case "${ID:-}:${ID_LIKE:-}" in
    ubuntu:*|debian:*|*:*debian*|*:*ubuntu*) ;;
    *)
        err "unsupported distro ID='${ID:-?}' ID_LIKE='${ID_LIKE:-?}'."
        err "this script targets Ubuntu/Debian. On other distros install the"
        err "equivalents of: gcc-riscv64-unknown-elf build-essential"
        err "libusb-1.0-0-dev libudev-dev pkg-config git make"
        exit 1
        ;;
esac
info "detected ${PRETTY_NAME:-$ID}"

# -- 2. Apt dependencies -----------------------------------------------------

info "installing apt build dependencies (sudo)"
sudo apt-get update -qq
sudo apt-get install -y \
    gcc-riscv64-unknown-elf \
    make \
    build-essential \
    libusb-1.0-0-dev \
    libudev-dev \
    git \
    pkg-config

# -- 3. udev rules -----------------------------------------------------------

if [[ ! -f "$UDEV_RULE_SRC" ]]; then
    err "missing $UDEV_RULE_SRC -- did you run this from a clean checkout?"
    exit 1
fi

info "installing udev rules to ${UDEV_RULE_DST}"
sudo install -m 0644 "$UDEV_RULE_SRC" "$UDEV_RULE_DST"
sudo udevadm control --reload-rules
sudo udevadm trigger || true   # 'trigger' can warn on WSL2; non-fatal

# -- 4. Done -----------------------------------------------------------------

cat <<'DONE'

setup.sh: complete.

Next steps:
  - Plug in your WCH-LinkE programmer.
  - cd apps/blink && make flash
  - The LED on the dev board should blink at ~2 Hz.

If you are on WSL2: udev rules only matter for native Linux. From the
Windows host run `usbipd attach --wsl --busid <id>` to expose the
WCH-LinkE to your WSL2 distro (see README "Programmer setup / WSL2").
DONE
