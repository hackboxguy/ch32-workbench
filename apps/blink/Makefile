TARGET        := blink
TARGET_MCU    := CH32V003
DEFAULT_BOARD ?= ch32v003f4p6_devboard
BOARD         ?= $(DEFAULT_BOARD)

# Path to ch32fun (submodule's inner directory holding ch32fun.mk).
CH32FUN := ../../ch32fun/ch32fun

# EXTRA_CFLAGS (not CFLAGS) is ch32fun.mk's hook for appending app-specific
# flags. Using CFLAGS+= here would preempt ch32fun.mk's `CFLAGS?=` default
# and silently strip -Os, -flto, -ffunction-sections, -fdata-sections.
#
# -Wextra: explicit so the warning level claimed in the README holds
# regardless of upstream ch32fun.mk defaults.
# -Wno-unused-parameter: silences three warnings from ch32fun's upstream
# setjmp/longjmp naked-function definitions (parameters are accessed via
# inline asm; the compiler can't see the use). Upstream is untouched per
# PRD, so we suppress at the build flag.
EXTRA_CFLAGS += -I../../boards -DBOARD_$(shell echo $(BOARD) | tr a-z A-Z) \
                -Wextra -Wno-unused-parameter

ADDITIONAL_C_FILES :=

include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean
