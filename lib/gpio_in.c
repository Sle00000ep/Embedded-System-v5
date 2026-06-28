/**
 * gpio_in.c — GPIO input driver (V5 adapted)
 *
 * IKEY0-3: PA2/PA3/PE2/PE3 — split across GPIOA+GPIOE
 * SW0-7:   PE4/PE5/PC13/PC14/PC15/PB0/PA4/PB1 — split across 4 ports
 *
 * V5 difference: V4 had all on PF0-PF11 (single port).
 * V5 fragments them across PA/PB/PC/PE due to 100-pin package.
 */

#include "gpio_in.h"
#include "pin_config.h"

/* ---- EXTI press flags (set by ISR, read & cleared by experiment) ---- */
volatile uint8_t ikey_pressed[4] = {0, 0, 0, 0};

/* ================================================================
 *  Polling mode — IKEY0-3 (Exp2)
 *  PA2/PA3 → GPIOA, PE2/PE3 → GPIOE
 * ================================================================ */
void IKEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* GPIOA: IKEY0(PA2) + IKEY1(PA3) */
    RCC_APB2PeriphClockCmd(IKEY0_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin   = IKEY0_PIN | IKEY1_PIN;
    GPIO_Init(IKEY0_PORT, &GPIO_InitStructure);

    /* GPIOE: IKEY2(PE2) + IKEY3(PE3) */
    RCC_APB2PeriphClockCmd(IKEY2_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin   = IKEY2_PIN | IKEY3_PIN;
    GPIO_Init(IKEY2_PORT, &GPIO_InitStructure);
}

uint8_t IKEY_Read(uint8_t idx)
{
    GPIO_TypeDef *port;
    uint16_t pin;

    switch (idx) {
        case 0: port = IKEY0_PORT; pin = IKEY0_PIN; break;
        case 1: port = IKEY1_PORT; pin = IKEY1_PIN; break;
        case 2: port = IKEY2_PORT; pin = IKEY2_PIN; break;
        case 3: port = IKEY3_PORT; pin = IKEY3_PIN; break;
        default: return 0;
    }
    return (GPIO_ReadInputDataBit(port, pin) == Bit_RESET) ? 1 : 0;
}

/* ================================================================
 *  EXTI interrupt mode — IKEY0-3 falling-edge (Exp8)
 *  ⚠️ IKEY0(PA2) and IKEY2(PE2) share EXTI2 → ISR needs combined check
 *  ⚠️ IKEY1(PA3) and IKEY3(PE3) share EXTI3 → ISR needs combined check
 * ================================================================ */
void IKEY_EXTI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* GPIOA: IKEY0(PA2) + IKEY1(PA3) */
    RCC_APB2PeriphClockCmd(IKEY0_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin   = IKEY0_PIN | IKEY1_PIN;
    GPIO_Init(IKEY0_PORT, &GPIO_InitStructure);

    /* GPIOE: IKEY2(PE2) + IKEY3(PE3) */
    RCC_APB2PeriphClockCmd(IKEY2_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin   = IKEY2_PIN | IKEY3_PIN;
    GPIO_Init(IKEY2_PORT, &GPIO_InitStructure);

    /* Map ONLY PE2→EXTI2, PE3→EXTI3 (unique EXTI per pin).
     * PA2 and PA3 are polled in Exp8 — they can't share EXTI2/3 with PE2/3
     * because STM32F1 EXTI only allows one port source per line. */
    GPIO_EXTILineConfig(IKEY2_EXTI_PORT, IKEY2_EXTI_PIN);  /* PE2 → EXTI2 */
    GPIO_EXTILineConfig(IKEY3_EXTI_PORT, IKEY3_EXTI_PIN);  /* PE3 → EXTI3 */

    /* EXTI lines 2-3 (both ports mapped to same lines) */
    EXTI_InitStructure.EXTI_Line    = EXTI_Line2 | EXTI_Line3;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* NVIC: EXTI2 */
    NVIC_InitStructure.NVIC_IRQChannel                   = EXTI2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority  = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority         = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                 = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* NVIC: EXTI3 */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
    NVIC_Init(&NVIC_InitStructure);

    EXTI_ClearITPendingBit(EXTI_Line2);
    EXTI_ClearITPendingBit(EXTI_Line3);

    ikey_pressed[0] = ikey_pressed[1] = ikey_pressed[2] = ikey_pressed[3] = 0;
}

void IKEY_EXTI_DeInit(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;

    EXTI_InitStructure.EXTI_Line    = EXTI_Line2 | EXTI_Line3;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = DISABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel                   = EXTI2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority  = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority         = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                 = DISABLE;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
    NVIC_Init(&NVIC_InitStructure);

    EXTI_ClearITPendingBit(EXTI_Line2);
    EXTI_ClearITPendingBit(EXTI_Line3);
}

/* ================================================================
 *  EXTI2-3 ISR — shared by two keys each on V5
 *  EXTI2: PA2(IKEY0) + PE2(IKEY2)
 *  EXTI3: PA3(IKEY1) + PE3(IKEY3)
 * ================================================================ */
void EXTI2_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line2) != RESET) {
        ikey_pressed[2] = 1;  /* PE2 = IKEY2 only */
        EXTI_ClearITPendingBit(EXTI_Line2);
    }
}

void EXTI3_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line3) != RESET) {
        ikey_pressed[3] = 1;  /* PE3 = IKEY3 only */
        EXTI_ClearITPendingBit(EXTI_Line3);
    }
}

/* ================================================================
 *  DIP Switch (Exp9) — split across PA/PB/PC/PE
 *  10K pull-down, ON=high
 * ================================================================ */
void SW_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;

    /* PE4(SW0) + PE5(SW1) */
    RCC_APB2PeriphClockCmd(SW0_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Pin = SW0_PIN | SW1_PIN;
    GPIO_Init(SW0_PORT, &GPIO_InitStructure);

    /* PC13(SW2) */
    RCC_APB2PeriphClockCmd(SW2_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Pin = SW2_PIN;
    GPIO_Init(SW2_PORT, &GPIO_InitStructure);

    /* PC14(SW3) + PC15(SW4) — RTC pins, need backup domain unlock */
    RCC_APB2PeriphClockCmd(SW3_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR->CR |= PWR_CR_DBP;  /* unlock backup domain (PC14/PC15 GPIO access) */
    GPIO_InitStructure.GPIO_Pin = SW3_PIN | SW4_PIN;
    GPIO_Init(SW3_PORT, &GPIO_InitStructure);

    /* PB0(SW5) */
    RCC_APB2PeriphClockCmd(SW5_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Pin = SW5_PIN;
    GPIO_Init(SW5_PORT, &GPIO_InitStructure);

    /* PA4(SW6) */
    RCC_APB2PeriphClockCmd(SW6_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Pin = SW6_PIN;
    GPIO_Init(SW6_PORT, &GPIO_InitStructure);

    /* PB1(SW7) */
    RCC_APB2PeriphClockCmd(SW7_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Pin = SW7_PIN;
    GPIO_Init(SW7_PORT, &GPIO_InitStructure);
}

uint8_t SW_ReadAll(void)
{
    uint8_t val = 0;

    if (GPIO_ReadInputDataBit(SW0_PORT, SW0_PIN) != Bit_RESET) val |= 0x01;
    if (GPIO_ReadInputDataBit(SW1_PORT, SW1_PIN) != Bit_RESET) val |= 0x02;
    if (GPIO_ReadInputDataBit(SW2_PORT, SW2_PIN) != Bit_RESET) val |= 0x04;
    if (GPIO_ReadInputDataBit(SW3_PORT, SW3_PIN) != Bit_RESET) val |= 0x08;
    if (GPIO_ReadInputDataBit(SW4_PORT, SW4_PIN) != Bit_RESET) val |= 0x10;
    if (GPIO_ReadInputDataBit(SW5_PORT, SW5_PIN) != Bit_RESET) val |= 0x20;
    if (GPIO_ReadInputDataBit(SW6_PORT, SW6_PIN) != Bit_RESET) val |= 0x40;
    if (GPIO_ReadInputDataBit(SW7_PORT, SW7_PIN) != Bit_RESET) val |= 0x80;

    return val;
}

uint8_t SW_Read(uint8_t idx)
{
    GPIO_TypeDef *port;
    uint16_t pin;

    switch (idx) {
        case 0: port = SW0_PORT; pin = SW0_PIN; break;
        case 1: port = SW1_PORT; pin = SW1_PIN; break;
        case 2: port = SW2_PORT; pin = SW2_PIN; break;
        case 3: port = SW3_PORT; pin = SW3_PIN; break;
        case 4: port = SW4_PORT; pin = SW4_PIN; break;
        case 5: port = SW5_PORT; pin = SW5_PIN; break;
        case 6: port = SW6_PORT; pin = SW6_PIN; break;
        case 7: port = SW7_PORT; pin = SW7_PIN; break;
        default: return 0;
    }
    return (GPIO_ReadInputDataBit(port, pin) != Bit_RESET) ? 1 : 0;
}
