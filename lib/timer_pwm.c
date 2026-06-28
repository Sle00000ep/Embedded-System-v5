/**
 * timer_pwm.c — DC Motor PWM + Servo (Exp19, Exp20)  [V5 adapted]
 *
 * Motor: PA6=TIM3_CH1, PA7=TIM3_CH2  — both PWM for H-bridge
 *   Forward:  CH1=PWM duty, CH2=0
 *   Reverse:  CH1=0, CH2=PWM duty
 *   Stop:     CH1=0, CH2=0
 * Servo: PB8=TIM4_CH3, 50Hz, 0°=500us, 180°=2500us
 */

#include "timer_pwm.h"
#include "pin_config.h"

/* ================================================================
 *  DC Motor — PA6(TIM3_CH1) + PA7(TIM3_CH2) both PWM
 * ================================================================ */
void Motor_Init(void)
{
    GPIO_InitTypeDef        GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;

    RCC_APB2PeriphClockCmd(PWMA_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(TIM3_SYS_CLK, ENABLE);

    /* PA6 = TIM3_CH1 AF_PP */
    GPIO_InitStructure.GPIO_Pin   = PWMA_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(PWMA_PORT, &GPIO_InitStructure);

    /* PA7 = TIM3_CH2 AF_PP */
    GPIO_InitStructure.GPIO_Pin   = PWMB_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(PWMB_PORT, &GPIO_InitStructure);

    /* TIM3: 1kHz PWM */
    TIM_TimeBaseStructure.TIM_Period        = 999;
    TIM_TimeBaseStructure.TIM_Prescaler     = 71;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    /* CH1 (PA6): PWM1, 0% initially */
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 0;
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);

    /* CH2 (PA7): PWM1, 0% initially */
    TIM_OCInitStructure.TIM_Pulse       = 0;
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);

    TIM_Cmd(TIM3, ENABLE);
}

static uint16_t motor_speed_ccr = 0;  /* stored CCR for direction switch */

void Motor_SetSpeed(uint8_t pct)
{
    if (pct > 100) pct = 100;
    motor_speed_ccr = (uint16_t)(((uint32_t)pct * 999) / 100);
    /* Active CH gets the duty */
    TIM_SetCompare1(TIM3, motor_speed_ccr);
}

/**
 * Set direction.
 *   dir=0 (forward):  CH1=PWM, CH2=0
 *   dir=1 (reverse):  CH1=0, CH2=PWM
 */
void Motor_SetDir(uint8_t dir)
{
    if (dir) {
        /* Reverse: CH2=speed, CH1=0 */
        TIM_SetCompare2(TIM3, motor_speed_ccr);
        TIM_SetCompare1(TIM3, 0);
    } else {
        /* Forward: CH1=speed, CH2=0 */
        TIM_SetCompare1(TIM3, motor_speed_ccr);
        TIM_SetCompare2(TIM3, 0);
    }
}

void Motor_Stop(void)
{
    TIM_SetCompare1(TIM3, 0);
    TIM_SetCompare2(TIM3, 0);
}

/* ================================================================
 *  LED PWM brightness control (TIM1_CH1 = PA8, P26)
 *  Uses TIM1 (independent from motor TIM3) to avoid motor conflict.
 * ================================================================ */
void LED_PWM_Init(void)
{
    GPIO_InitTypeDef        GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1, ENABLE);

    /* PA8 = TIM1_CH1 AF_PP */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* TIM1: 1kHz PWM, 72MHz/72=1MHz, 1MHz/1000=1kHz */
    TIM_TimeBaseStructure.TIM_Period        = 999;
    TIM_TimeBaseStructure.TIM_Prescaler     = 71;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    /* CH1: PWM1, 0% initially */
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 0;
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);

    /* TIM1 advanced timer: need MOE (Main Output Enable) */
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

void LED_PWM_SetBrightness(uint8_t pct)
{
    uint16_t ccr;
    if (pct > 100) pct = 100;
    ccr = (uint16_t)(((uint32_t)pct * 999) / 100);
    TIM_SetCompare1(TIM1, ccr);
}

/* ================================================================
 *  TIM4 Servo — PB8 (TIM4_CH3), 50Hz hardware PWM
 * ================================================================ */
void TIM4_Servo_Init(void)
{
    GPIO_InitTypeDef        GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;

    TIM_Cmd(TIM4, DISABLE);  /* stop timer before reconfig */

    RCC_APB2PeriphClockCmd(SERVO_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(TIM4_SYS_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = SERVO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(SERVO_PORT, &GPIO_InitStructure);

    /* 50Hz: 72MHz / 72 = 1MHz, 1MHz / 20000 = 50Hz */
    TIM_TimeBaseStructure.TIM_Period        = 19999;
    TIM_TimeBaseStructure.TIM_Prescaler     = 71;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 1500;  /* center */
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC3Init(TIM4, &TIM_OCInitStructure);

    TIM_SetCounter(TIM4, 0);  /* reset counter */
    TIM_Cmd(TIM4, ENABLE);
}

void TIM4_Servo_SetAngle(uint8_t angle)
{
    uint16_t pulse;
    if (angle > 180) angle = 180;
    pulse = 500 + ((uint32_t)angle * 2000) / 180;
    TIM_SetCompare3(TIM4, pulse);
}
