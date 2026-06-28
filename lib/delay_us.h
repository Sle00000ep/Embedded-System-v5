/**
 * delay_us.h — microsecond delay using DWT cycle counter (Cortex-M3)
 *
 * Requires: delay_us_init() once before use.
 * SystemCoreClock must be set (typically 72MHz for STM32F103).
 */

#ifndef __DELAY_US_H
#define __DELAY_US_H

#include "stm32f10x.h"

void delay_us_init(void);
void delay_us(uint32_t us);

#endif /* __DELAY_US_H */
