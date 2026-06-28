/**
 * gpio_out.h — GPIO输出驱动头文件
 */

#ifndef __GPIO_OUT_H
#define __GPIO_OUT_H

#include "stm32f10x.h"

void LED_Init(void);
void LED_WriteAll(uint8_t val);
void LED_Set(uint8_t index, uint8_t on);
void LED_AllOff(void);
void LED_AllOn(void);

void TrafficLight_Init(void);
void TrafficLight_Set(uint8_t ns, uint8_t we);
void Buzzer_Set(uint8_t on);

#endif /* __GPIO_OUT_H */
