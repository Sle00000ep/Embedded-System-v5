/**
 * stepper.h — 28BYJ-48 4-phase stepper motor driver header (Exp18)
 *
 * PE12 = PHASE-A (ULN2003 IN1, orange)
 * PE13 = PHASE-B (ULN2003 IN2, yellow)
 * PE14 = PHASE-C (ULN2003 IN3, pink)
 * PE15 = PHASE-D (ULN2003 IN4, blue)
 *
 * 28BYJ-48 specs:
 *   - Unipolar 5V, 64:1 gear ratio
 *   - Half-step (8-beat): 4096 steps/rev
 *   - Step angle: 5.625°/64 = ~0.088° per half-step
 */

#ifndef __STEPPER_H
#define __STEPPER_H

#include "stm32f10x.h"

void Stepper_Init(void);
void Stepper_StepCW(uint16_t steps);     /* clockwise, half-step */
void Stepper_StepCCW(uint16_t steps);    /* counter-clockwise, half-step */
void Stepper_Stop(void);                 /* all phases off */

#endif /* __STEPPER_H */
