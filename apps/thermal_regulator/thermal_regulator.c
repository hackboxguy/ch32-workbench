/*
 * thermal_regulator -- closed-loop PWM fan controller for CH32V003.
 *
 * A DS18B20 temperature sensor drives the duty cycle of a Noctua NF-A8
 * 12 V 4-pin PWM fan; the fan's tachometer is read back; temperature,
 * applied duty, and fan speed are shown on an SSD1306 I2C OLED and also
 * logged once per second over the SWIO debug channel.
 *
 * Ported from an ESP32-C3 reference. Differences forced by the CH32V003:
 *   - bare-metal superloop instead of a FreeRTOS task
 *   - hardware TIM2 PWM instead of the ESP32 LEDC peripheral
 *   - EXTI falling-edge interrupt for the tachometer
 *   - DS18B20 over a bit-banged 1-Wire bus (ch32fun's static_onewire.h)
 *   - all math is fixed-point integer (no FPU on this part):
 *     temperature is carried in centidegrees C (3181 == 31.81 C)
 *
 * --- Pinout (CH32V003F4P6) ---------------------------------------------
 *   PC1   I2C1 SDA   -> OLED SDA
 *   PC2   I2C1 SCL   -> OLED SCL
 *   PD4   TIM2_CH1   -> fan pin 4 (PWM, blue),  25 kHz, 3.3 V
 *   PD0   EXTI0 in   -> fan pin 3 (tach, green), 10 k pull-up to 3V3
 *   PD3   1-Wire     -> DS18B20 DQ,              4.7 k pull-up to 3V3
 *   PD1   SWIO       -> WCH-LinkE (programming/debug)
 *
 * --- Critical hardware notes -------------------------------------------
 *   - Common ground is non-negotiable: tie the 12 V PSU negative, the
 *     fan GND, and the board GND together at one point with short wire.
 *     Ground bounce on the tach line otherwise inflates the count wildly.
 *   - The tach pull-up MUST go to 3.3 V (open-collector output) -- 5 V or
 *     12 V will damage the input pin.
 *   - 3.3 V PWM drives Noctua fans directly (no level shifter needed).
 *   - Fan +12 V (pin 2, yellow) comes from a separate 12 V supply.
 */

#define SSD1306_128X64

#include "ch32fun.h"
#include <stdio.h>
#include "ssd1306_i2c.h"
#include "ssd1306.h"

/* --- Pin assignment --------------------------------------------------- */
#define PIN_FAN_PWM  PD4 /* TIM2_CH1 (default mapping) */
#define PIN_FAN_TACH PD0 /* EXTI line 0 */
#define PIN_ONEWIRE  PD3 /* DS18B20 data */

/* --- PWM: 25 kHz on TIM2_CH1 ------------------------------------------ */
/* 48 MHz / (PSC+1) / (ARR+1) = 48e6 / 1 / 1920 = 25 kHz exactly. */
#define PWM_ARR 1919 /* counter wraps 0..1919 */
#define PWM_TOP 1920 /* CH1CVR == PWM_TOP -> 100% (never below compare) */

/* --- Fan curve (centidegrees C) --------------------------------------- */
#define TEMP_MIN_CC   3000 /* 30.00 C and below -> idle duty */
#define TEMP_MAX_CC   6000 /* 60.00 C and above -> full duty */
#define DUTY_IDLE_PCT 20   /* above the fan's stall point */
#define DUTY_FULL_PCT 100
#define DUTY_HYST_PCT 3         /* only re-apply PWM on a >=3% move */
#define TEMP_BAD_CC   (-100000) /* sentinel: sensor read failed */

/* --- DS18B20 1-Wire commands ------------------------------------------ */
#define DS_SKIP_ROM  0xCC
#define DS_CONVERT_T 0x44
#define DS_READ_SCR  0xBE

/* --- 1-Wire glue for ch32fun's static_onewire.h ----------------------- */
#define ONEPREFIX one
#define DELAY(n)  Delay_Us(n)
#define ONE_INPUT                                                                                  \
    {                                                                                              \
        funPinMode(PIN_ONEWIRE, GPIO_CFGLR_IN_PUPD);                                               \
        funDigitalWrite(PIN_ONEWIRE, 1);                                                           \
    }
#define ONE_OUTPUT                                                                                 \
    {                                                                                              \
        funDigitalWrite(PIN_ONEWIRE, 0);                                                           \
        funPinMode(PIN_ONEWIRE, GPIO_CFGLR_OUT_2Mhz_PP);                                           \
    }
#define ONE_SET                                                                                    \
    { funDigitalWrite(PIN_ONEWIRE, 1); }
#define ONE_CLEAR                                                                                  \
    { funDigitalWrite(PIN_ONEWIRE, 0); }
#define ONE_READ          funDigitalRead(PIN_ONEWIRE)
#define ONENEEDCRC8_TABLE 1
#include "static_onewire.h"

/* --- Tachometer ------------------------------------------------------- */
/*
 * Falling edges on PIN_FAN_TACH increment tach_count. A 1 ms inter-edge
 * filter rejects contact-bounce / ground-noise: the NF-A8's real maximum
 * edge rate is ~73 Hz (2200 RPM, 2 pulses/rev), so 1 ms leaves >13x
 * margin against legitimate edges.
 */
static volatile uint32_t tach_count = 0;

void EXTI7_0_IRQHandler(void) __attribute__((interrupt));
void EXTI7_0_IRQHandler(void) {
    static uint32_t last_tick = 0;
    if (EXTI->INTFR & EXTI_Line0) {
        EXTI->INTFR = EXTI_Line0; /* acknowledge */
        uint32_t now = SysTick->CNT;
        if ((uint32_t)(now - last_tick) >= Ticks_from_Ms(1)) {
            last_tick = now;
            tach_count++;
        }
    }
}

static void tach_init(void) {
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO;
    /* input with pull, ODR=1 selects pull-UP (idle-high, open-collector) */
    funPinMode(PIN_FAN_TACH, GPIO_CFGLR_IN_PUPD);
    funDigitalWrite(PIN_FAN_TACH, 1);
    AFIO->EXTICR |= AFIO_EXTICR_EXTI0_PD; /* route EXTI0 to port D */
    EXTI->INTENR |= EXTI_INTENR_MR0;      /* unmask line 0 */
    EXTI->FTENR |= EXTI_FTENR_TR0;        /* trigger on falling edge */
    NVIC_EnableIRQ(EXTI7_0_IRQn);
}

/* --- PWM -------------------------------------------------------------- */
static void pwm_init(void) {
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;
    RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

    /* PD4 = TIM2_CH1, alt-function push-pull output */
    funPinMode(PIN_FAN_PWM, GPIO_CFGLR_OUT_10Mhz_AF_PP);

    /* reset TIM2 to a known state */
    RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
    RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;

    TIM2->PSC = 0;
    TIM2->ATRLR = PWM_ARR;
    /* CH1: PWM mode 1 (OC1M=110), output-compare preload enabled */
    TIM2->CHCTLR1 |= TIM_OC1M_2 | TIM_OC1M_1 | TIM_OC1PE;
    TIM2->CTLR1 |= TIM_ARPE; /* auto-reload preload */
    TIM2->CCER |= TIM_CC1E;  /* enable CH1 output, non-inverted */
    TIM2->CH1CVR = 0;        /* start at 0% */
    TIM2->SWEVGR |= TIM_UG;  /* load preloaded registers */
    TIM2->CTLR1 |= TIM_CEN;  /* start the timer */
}

static void pwm_set_pct(int pct) {
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;
    TIM2->CH1CVR = (uint16_t)((pct * PWM_TOP) / 100);
}

/* --- DS18B20 ---------------------------------------------------------- */
/* Kick off a temperature conversion (addressed via SKIP ROM: one sensor). */
static void ds18b20_start_conversion(void) {
    if (oneReset() != 0) /* nonzero == no presence pulse */
        return;
    oneSendByte(DS_SKIP_ROM);
    oneSendByte(DS_CONVERT_T);
}

/* Read the last conversion result. Returns 0 and fills *out_cc with the
 * temperature in centidegrees C; returns nonzero on no-presence or CRC
 * failure. */
static int ds18b20_read(int *out_cc) {
    if (oneReset() != 0)
        return -1;
    oneSendByte(DS_SKIP_ROM);
    oneSendByte(DS_READ_SCR);

    uint8_t scr[9];
    for (int i = 0; i < 9; i++)
        scr[i] = oneGetByte();

    if (oneCRC8(0, scr, 8) != scr[8])
        return -2;

    /* scratchpad bytes 0,1 are the signed 16-bit raw value, 1/16 C/LSB */
    int16_t raw = (int16_t)((scr[1] << 8) | scr[0]);
    *out_cc = ((int)raw * 100) / 16;
    return 0;
}

/* --- Control ---------------------------------------------------------- */
static int iabs(int v) {
    return v < 0 ? -v : v;
}

/* Map temperature (centidegrees) to a fan duty percentage. On a failed
 * sensor read, return full duty as the safe fallback. */
static int temp_to_duty_pct(int temp_cc) {
    if (temp_cc <= TEMP_BAD_CC)
        return DUTY_FULL_PCT;
    if (temp_cc <= TEMP_MIN_CC)
        return DUTY_IDLE_PCT;
    if (temp_cc >= TEMP_MAX_CC)
        return DUTY_FULL_PCT;
    int above = temp_cc - TEMP_MIN_CC;             /* 0 .. 3000  */
    int duty_span = DUTY_FULL_PCT - DUTY_IDLE_PCT; /* 80         */
    int temp_span = TEMP_MAX_CC - TEMP_MIN_CC;     /* 3000       */
    return DUTY_IDLE_PCT + (above * duty_span) / temp_span;
}

/* --- Output ----------------------------------------------------------- */
/* Split signed centidegrees into sign / whole / two-digit fraction. */
static void temp_parts(int temp_cc, const char **sign, int *whole, int *frac) {
    int neg = temp_cc < 0;
    int mag = neg ? -temp_cc : temp_cc;
    *sign = neg ? "-" : "";
    *whole = mag / 100;
    *frac = mag % 100;
}

static void oled_show(int temp_cc, int duty_pct, unsigned tps, unsigned rpm) {
    char line[24];

    ssd1306_setbuf(0);
    ssd1306_drawstr(0, 0, "FAN REGULATOR", 1);

    if (temp_cc <= TEMP_BAD_CC) {
        ssd1306_drawstr(0, 14, "Temp:  ERR", 1);
    } else {
        const char *sign;
        int whole, frac;
        temp_parts(temp_cc, &sign, &whole, &frac);
        snprintf(line, sizeof(line), "Temp:  %s%d.%02d C", sign, whole, frac);
        ssd1306_drawstr(0, 14, line, 1);
    }

    snprintf(line, sizeof(line), "Duty:  %d%%", duty_pct);
    ssd1306_drawstr(0, 26, line, 1);

    snprintf(line, sizeof(line), "Tach:  %u t/s", tps);
    ssd1306_drawstr(0, 38, line, 1);

    snprintf(line, sizeof(line), "Fan:   %u RPM", rpm);
    ssd1306_drawstr(0, 50, line, 1);

    ssd1306_refresh();
}

static void debug_log(int temp_cc, int duty_pct, unsigned tps, unsigned rpm) {
    if (temp_cc <= TEMP_BAD_CC) {
        printf("T=ERR       duty=%3d%%  tach=%4u t/s  rpm=%5u\n", duty_pct, tps, rpm);
    } else {
        const char *sign;
        int whole, frac;
        temp_parts(temp_cc, &sign, &whole, &frac);
        printf("T=%s%d.%02d C  duty=%3d%%  tach=%4u t/s  rpm=%5u\n", sign, whole, frac, duty_pct,
               tps, rpm);
    }
}

/* --- Main ------------------------------------------------------------- */
int main(void) {
    SystemInit();
    Delay_Ms(100);

    int oled_ok = (ssd1306_i2c_init() == 0);
    if (oled_ok) {
        ssd1306_init();
        ssd1306_setbuf(0);
        ssd1306_drawstr(0, 0, "FAN REGULATOR", 1);
        ssd1306_drawstr(0, 24, "starting...", 1);
        ssd1306_refresh();
    }

    pwm_init();
    tach_init();
    ONE_INPUT; /* park the 1-Wire pin idle-high */

    pwm_set_pct(DUTY_IDLE_PCT);
    int duty_applied = DUTY_IDLE_PCT;

    printf("thermal_regulator: starting (OLED %s)\n", oled_ok ? "ok" : "absent");

    /* The control window is delimited by SysTick so the tach rate is
     * normalised to a true per-second figure regardless of how long the
     * sensor read / display refresh actually take. */
    uint32_t win_t0 = SysTick->CNT;
    uint32_t win_c0 = tach_count;

    while (1) {
        /* 1. trigger a conversion and wait it out (12-bit <= 750 ms) */
        ds18b20_start_conversion();
        Delay_Ms(800);

        /* 2. read the sensor (safe fallback to full duty on failure) */
        int temp_cc;
        if (ds18b20_read(&temp_cc) != 0)
            temp_cc = TEMP_BAD_CC;

        /* 3. control: map temp -> duty, apply through a hysteresis band */
        int duty_target = temp_to_duty_pct(temp_cc);
        if (iabs(duty_target - duty_applied) >= DUTY_HYST_PCT) {
            duty_applied = duty_target;
            pwm_set_pct(duty_applied);
        }

        /* 4. hold the window open until at least 1 s has elapsed */
        while ((uint32_t)(SysTick->CNT - win_t0) < Ticks_from_Ms(1000)) {
        }
        uint32_t win_t1 = SysTick->CNT;
        uint32_t win_c1 = tach_count;
        uint32_t elapsed_ms = (uint32_t)(win_t1 - win_t0) / Ticks_from_Ms(1);
        uint32_t ticks = win_c1 - win_c0;
        unsigned tps = elapsed_ms ? (unsigned)((ticks * 1000u) / elapsed_ms) : 0;
        unsigned rpm = tps * 30u; /* NF-A8: 2 tach pulses per revolution */

        /* 5. report */
        if (oled_ok)
            oled_show(temp_cc, duty_applied, tps, rpm);
        debug_log(temp_cc, duty_applied, tps, rpm);

        /* next window starts exactly where this one closed -- no gap, no
         * double-counted edges */
        win_t0 = win_t1;
        win_c0 = win_c1;
    }
}
