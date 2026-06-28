/**
 * encoder.h — Pulse + Direction encoder header (Exp18)
 *
 * PB6 = OUT (EXTI6 rising edge → count ±1 based on DIR)
 * PB7 = DIR (HIGH=CW +1, LOW=CCW -1)
 * PD15 = Z (LOW=reset)
 */

#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f10x.h"

extern volatile int32_t enc_count;

void    ENC_Init(void);
int32_t ENC_GetCount(void);
void    ENC_Reset(void);
uint8_t ENC_Z_IsPressed(void);

#endif /* __ENCODER_H */
