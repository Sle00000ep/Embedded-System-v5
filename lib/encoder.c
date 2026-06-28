/**
 * encoder.c — Pulse + Direction encoder counter (Exp18)
 *
 * PB6 (OUT): EXTI6 rising edge → ISR reads PB7(DIR) → enc_count ±= 1
 * PB7 (DIR): GPIO input, HIGH=CW(+1) LOW=CCW(-1)
 * PD15 (Z):  GPIO input, LOW=pressed → reset enc_count = 0
 */

#include "encoder.h"
#include "pin_config.h"

volatile int32_t enc_count = 0;

void ENC_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    EXTI_InitTypeDef  EXTI_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(ENC_A_CLK, ENABLE);   /* GPIOB */
    RCC_APB2PeriphClockCmd(ENC_Z_CLK, ENABLE);   /* GPIOD */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* PB6 = OUT: input pull-up, EXTI rising edge */
    GPIO_InitStructure.GPIO_Pin  = ENC_A_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(ENC_A_PORT, &GPIO_InitStructure);

    /* PB7 = DIR: input pull-up (HIGH=CW, LOW=CCW) */
    GPIO_InitStructure.GPIO_Pin  = ENC_B_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(ENC_B_PORT, &GPIO_InitStructure);

    /* PD15 = Z: input pull-up (LOW=reset) */
    GPIO_InitStructure.GPIO_Pin  = ENC_Z_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(ENC_Z_PORT, &GPIO_InitStructure);

    /* PB6 → EXTI6 (rising edge counts on pulse) */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource6);

    EXTI_InitStructure.EXTI_Line    = EXTI_Line6;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel                   = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority  = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority         = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                 = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    enc_count = 0;
}

int32_t ENC_GetCount(void)
{
    return enc_count;
}

void ENC_Reset(void)
{
    enc_count = 0;
}

/* Z phase check — call from experiment loop */
uint8_t ENC_Z_IsPressed(void)
{
    return (GPIO_ReadInputDataBit(ENC_Z_PORT, ENC_Z_PIN) == Bit_RESET) ? 1 : 0;
}

/* EXTI9_5 IRQ: PB6(EXTI6) rising edge */
void EXTI9_5_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line6) != RESET) {
        if (GPIO_ReadInputDataBit(ENC_B_PORT, ENC_B_PIN) != Bit_RESET)
            enc_count++;   /* DIR=HIGH → CW */
        else
            enc_count--;   /* DIR=LOW → CCW */
        EXTI_ClearITPendingBit(EXTI_Line6);
    }
}
