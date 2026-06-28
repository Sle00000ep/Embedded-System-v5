/**
 * stepper.c — 28BYJ-48 stepper motor 8-beat half-step driver (Exp18)
 *
 * PE12=PHASE-A, PE13=PHASE-B, PE14=PHASE-C, PE15=PHASE-D
 * ULN2003 Darlington driver: GPIO high → coil energized
 *
 * Half-step sequence (8 beats per electrical cycle):
 *   [1 0 0 0] [1 1 0 0] [0 1 0 0] [0 1 1 0]
 *   [0 0 1 0] [0 0 1 1] [0 0 0 1] [1 0 0 1]
 *
 * 28BYJ-48: 64 steps/rev motor × 64:1 gear = 4096 half-steps/rev output shaft
 */

#include "stepper.h"
#include "pin_config.h"
#include "delay.h"
#include <stdio.h>

/* 8-beat half-step: [A B C D] */
static const uint8_t step_table[8] = {
    0x01,   /* 0001 — A on */
    0x03,   /* 0011 — A+B on */
    0x02,   /* 0010 — B on */
    0x06,   /* 0110 — B+C on */
    0x04,   /* 0100 — C on */
    0x0C,   /* 1100 — C+D on */
    0x08,   /* 1000 — D on */
    0x09,   /* 1001 — D+A on */
};

/* Pin-to-bit mapping: PE12=bit0(A), PE13=bit1(B), PE14=bit2(C), PE15=bit3(D) */
static void Stepper_Output(uint8_t phase_bits)
{
    if (phase_bits & 0x01)
        GPIO_SetBits(STEP_A_PORT, STEP_A_PIN);
    else
        GPIO_ResetBits(STEP_A_PORT, STEP_A_PIN);

    if (phase_bits & 0x02)
        GPIO_SetBits(STEP_B_PORT, STEP_B_PIN);
    else
        GPIO_ResetBits(STEP_B_PORT, STEP_B_PIN);

    if (phase_bits & 0x04)
        GPIO_SetBits(STEP_C_PORT, STEP_C_PIN);
    else
        GPIO_ResetBits(STEP_C_PORT, STEP_C_PIN);

    if (phase_bits & 0x08)
        GPIO_SetBits(STEP_D_PORT, STEP_D_PIN);
    else
        GPIO_ResetBits(STEP_D_PORT, STEP_D_PIN);
}

void Stepper_Init(void)
{
    GPIO_InitTypeDef s;

    RCC_APB2PeriphClockCmd(STEP_A_CLK, ENABLE);   /* GPIOE */

    s.GPIO_Pin   = STEPPER_PINS;
    s.GPIO_Speed = GPIO_Speed_50MHz;
    s.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(STEPPER_PORT, &s);

    /* Power-on self-test: all 4 phases HIGH for 500ms.
     * Measure PE12/PE13/PE14/PE15 with multimeter → should all be ~3.3V */
    GPIO_SetBits(STEPPER_PORT, STEPPER_PINS);
    delay_ms(500);
    Stepper_Stop();
}

void Stepper_Stop(void)
{
    GPIO_ResetBits(STEPPER_PORT, STEPPER_PINS);
}

/**
 * Step CW (clockwise).
 * Each step: 5ms delay → ~100 half-steps/sec → ~40 sec/rev
 */
void Stepper_StepCW(uint16_t steps)
{
    uint16_t i;
    uint8_t beat;

    printf("  Stepper CW: %d steps start...\r\n", steps);
    for (i = 0; i < steps; i++) {
        beat = i % 8;
        Stepper_Output(step_table[beat]);
        delay_ms(5);   /* 5ms per half-step */
        if ((i & 0x3F) == 0x3F)   /* every 64 steps */
            printf("  Step CW: %d/%d\r\n", i + 1, steps);
    }
    Stepper_Stop();
    printf("  Stepper CW: done (phases LOW)\r\n");
}

/**
 * Step CCW (counter-clockwise).
 */
void Stepper_StepCCW(uint16_t steps)
{
    uint16_t i;
    uint8_t beat;

    printf("  Stepper CCW: %d steps start...\r\n", steps);
    for (i = 0; i < steps; i++) {
        beat = (7 - (i % 8));  /* reverse the sequence */
        Stepper_Output(step_table[beat]);
        delay_ms(5);   /* 5ms per half-step */
        if ((i & 0x3F) == 0x3F)
            printf("  Step CCW: %d/%d\r\n", i + 1, steps);
    }
    Stepper_Stop();
    printf("  Stepper CCW: done (phases LOW)\r\n");
}
