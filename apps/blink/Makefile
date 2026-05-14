TARGET        := blink
TARGET_MCU    := CH32V003
DEFAULT_BOARD ?= ch32v003f4p6_devboard
BOARD         ?= $(DEFAULT_BOARD)

# Path to ch32fun (submodule's inner directory holding ch32fun.mk).
CH32FUN := ../../ch32fun/ch32fun

# Pass board selection to the compiler so boards/board.h can pick the right
# include. -Wextra is added explicitly so the warning level claimed in the
# README holds regardless of upstream ch32fun.mk defaults.
# -Wno-unused-parameter silences three warnings from ch32fun's upstream
# setjmp/longjmp naked-function definitions; those parameters are accessed
# via inline asm and the compiler can't see the use. Upstream is untouched
# per PRD, so we suppress at the build flag.
CFLAGS  += -I../../boards -DBOARD_$(shell echo $(BOARD) | tr a-z A-Z) \
           -Wextra -Wno-unused-parameter

ADDITIONAL_C_FILES :=

include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean
