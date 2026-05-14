TARGET        := gpio_button
TARGET_MCU    := CH32V003
DEFAULT_BOARD ?= ch32v003f4p6_devboard
BOARD         ?= $(DEFAULT_BOARD)

CH32FUN := ../../ch32fun/ch32fun

# See apps/blink/Makefile for why EXTRA_CFLAGS, not CFLAGS.
EXTRA_CFLAGS += -I../../boards -DBOARD_$(shell echo $(BOARD) | tr a-z A-Z) \
                -Wextra -Wno-unused-parameter

ADDITIONAL_C_FILES :=

include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean
