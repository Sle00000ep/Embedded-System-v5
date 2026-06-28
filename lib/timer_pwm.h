/**
 * timer_pwm.h — DC Motor PWM + Servo header (Exp19, Exp20)
 *
 * Motor: PA6(TIM3_CH1) + PA7(TIM3_CH2) — both PWM for H-bridge
 * Servo: PA8, software PWM 50Hz, 0.5-2.5ms pulse
 */

#ifndef __TIMER_PWM_H
#define __TIMER_PWM_H

#include "stm32f10x.h"

/* ---- DC Motor (PA6=PWM, PA7=PWM) ---- */
void Motor_Init(void);
void Motor_SetSpeed(uint8_t pct);   /* 0-100% duty */
void Motor_SetDir(uint8_t dir);     /* 0=forward, 1=reverse */
void Motor_Stop(void);

/* ---- LED PWM brightness (PA6=TIM3_CH1) ---- */
void LED_PWM_Init(void);
void LED_PWM_SetBrightness(uint8_t pct);

/* ---- Servo (PB8, TIM4_CH3, 50Hz PWM) ---- */
void TIM4_Servo_Init(void);
void TIM4_Servo_SetAngle(uint8_t angle); /* 0-180 degrees */

#endif /* __TIMER_PWM_H */
