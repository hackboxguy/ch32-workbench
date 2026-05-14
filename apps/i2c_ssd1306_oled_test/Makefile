TARGET        := i2c_ssd1306_oled_test
TARGET_MCU    := CH32V003
DEFAULT_BOARD ?= ch32v003f4p6_devboard
BOARD         ?= $(DEFAULT_BOARD)

CH32FUN := ../../ch32fun/ch32fun

# Upstream warnings we explicitly suppress (we don't modify ch32fun/):
#   -Wno-unused-parameter:    setjmp/longjmp naked function defs
#   -Wno-sign-compare:        extralibs/ssd1306.h x>128-8*font_size
#   -Wno-old-style-declaration: extralibs/font_8x8.h "static const" order
EXTRA_CFLAGS += -I../../boards -DBOARD_$(shell echo $(BOARD) | tr a-z A-Z) \
                -Wextra -Wno-unused-parameter \
                -Wno-sign-compare -Wno-old-style-declaration

ADDITIONAL_C_FILES :=

include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean
