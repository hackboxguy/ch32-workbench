TARGET        := uart_hello
TARGET_MCU    := CH32V003
DEFAULT_BOARD ?= ch32v003f4p6_devboard
BOARD         ?= $(DEFAULT_BOARD)

CH32FUN := ../../ch32fun/ch32fun

# EXTRA_CFLAGS (not CFLAGS) is ch32fun.mk's hook for appending app-specific
# flags. Using CFLAGS+= here would preempt ch32fun.mk's `CFLAGS?=` default
# and silently strip -Os, -flto, -ffunction-sections, -fdata-sections --
# breaking link-time dead-code elimination of ch32fun's WEAK printf path.
EXTRA_CFLAGS += -I../../boards -DBOARD_$(shell echo $(BOARD) | tr a-z A-Z) \
                -Wextra -Wno-unused-parameter

ADDITIONAL_C_FILES :=

include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean
