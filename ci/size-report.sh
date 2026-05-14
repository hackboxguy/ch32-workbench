#!/usr/bin/env bash
#
# ci/size-report.sh — build every app × every board, capture flash/SRAM
# usage from `riscv64-unknown-elf-size`, and emit a markdown table.
#
# Designed for two callers:
#   - GitHub Actions, where the table is appended to $GITHUB_STEP_SUMMARY
#   - a developer locally, where the table prints to stdout
#
# Side effect: leaves clean working tree (runs `make clean` at the end).

set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT"

APPS=$(ls apps)
BOARDS=$(find boards -maxdepth 1 -name '*.h' ! -name 'board.h' \
            -printf '%f\n' | sed 's/\.h$//')

if [[ -z "$APPS" || -z "$BOARDS" ]]; then
    echo "size-report.sh: no apps or boards found under $ROOT" >&2
    exit 1
fi

# Build each combination, capture size data into an associative array.
declare -A FLASH SRAM
fail=0
for app in $APPS; do
    for board in $BOARDS; do
        (cd "apps/$app" && make clean -s >/dev/null 2>&1)
        if ! (cd "apps/$app" && make BOARD="$board" -s >/dev/null 2>&1); then
            FLASH["$app:$board"]="BUILD-FAIL"
            SRAM["$app:$board"]="BUILD-FAIL"
            fail=1
            continue
        fi
        elf="apps/$app/$app.elf"
        # `size` line format: text data bss dec hex filename
        read -r text data bss _ < <(riscv64-unknown-elf-size "$elf" \
                                        | awk 'NR==2 { print $1, $2, $3 }')
        FLASH["$app:$board"]=$((text + data))
        SRAM["$app:$board"]=$((data + bss))
    done
done

# Emit the markdown table.
{
    printf '## Build size report\n\n'
    printf '| App | Board | Flash (B) | SRAM (B) | Flash %% of 16 KB |\n'
    printf '|---|---|---:|---:|---:|\n'
    for app in $APPS; do
        for board in $BOARDS; do
            f=${FLASH[$app:$board]}
            s=${SRAM[$app:$board]}
            if [[ "$f" == "BUILD-FAIL" ]]; then
                printf '| %s | %s | **FAIL** | **FAIL** | — |\n' "$app" "$board"
            else
                pct=$(awk "BEGIN { printf \"%.2f\", ($f * 100) / 16384 }")
                printf '| %s | %s | %s | %s | %s%% |\n' "$app" "$board" "$f" "$s" "$pct"
            fi
        done
    done
} | tee -a "${GITHUB_STEP_SUMMARY:-/dev/null}"

# Final cleanup so the working tree stays tidy.
for app in $APPS; do
    (cd "apps/$app" && make clean -s >/dev/null 2>&1) || true
done

exit $fail
