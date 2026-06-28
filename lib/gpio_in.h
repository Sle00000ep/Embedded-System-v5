/**
 * gpio_in.h — GPIO input driver header
 *
 * IKEY0-3 (PF0-PF3) — MainBoard independent keys, active-low, 10K pull-up
 * SW0-7   (PF4-PF11) — MainBoard DIP switches, active-high, 10K pull-down
 * WKUP    (PA0)     — Core board built-in key, already handled in main.c
 *
 * Pin sharing: PF0-PF3 = LED1-4 | IKEY0-3, PF4-PF7 = LED5-8 | SW0-3
 * Each experiment reinitializes GPIODIR as needed.
 */

#ifndef __GPIO_IN_H
#define __GPIO_IN_H

#include "stm32f10x.h"

/* EXTI press flags — set by ISR, read & cleared by experiment loop */
extern volatile uint8_t ikey_pressed[4];

/* ---- Polling mode (Exp2) ---- */
void IKEY_Init(void);
uint8_t IKEY_Read(uint8_t idx);   /* idx 0-3, returns 1 if pressed (low) */

/* ---- EXTI interrupt mode (Exp8) ---- */
void IKEY_EXTI_Init(void);
void IKEY_EXTI_DeInit(void);      /* disable EXTI lines + NVIC, keep IPU */

/* ---- DIP switches (Exp9) ---- */
void SW_Init(void);
uint8_t SW_ReadAll(void);         /* return 8-bit: bit0=SW0 ... bit7=SW7 */
uint8_t SW_Read(uint8_t idx);     /* idx 0-7, returns 1 if ON (high) */

#endif /* __GPIO_IN_H */
