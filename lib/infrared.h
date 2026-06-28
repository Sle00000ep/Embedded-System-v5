/**
 * infrared.h — HS0038 NEC infrared decoder header (Exp17)
 *
 * PD12, EXTI12 falling edge. ISR sets ir_received flag; main loop
 * calls IR_Decode() which reads the pulse train with delay_us().
 */

#ifndef __INFRARED_H
#define __INFRARED_H

#include "stm32f10x.h"

extern volatile uint8_t ir_received;

typedef struct {
    uint8_t  addr;
    uint8_t  cmd;
    uint8_t  is_repeat;
    uint8_t  valid;       /* 1=ok, 0=invalid/timeout */
} IR_Data;

void IR_Init(void);           /* GPIO + EXTI init */
void IR_EXTI_Disable(void);   /* disable EXTI during decode */
void IR_EXTI_Enable(void);    /* re-enable after decode */
IR_Data IR_Decode(void);      /* blocking decode (call when ir_received==1) */

#endif /* __INFRARED_H */
