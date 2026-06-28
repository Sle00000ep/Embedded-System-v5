#ifndef __TSENSOR_H
#define __TSENSOR_H
#include "stm32f10x.h"

short Get_Temprate(void);
void T_Adc_Init(void);
u16  T_Get_Adc(u8 ch);
u16  T_Get_Adc_Average(u8 ch, u8 times);

void Adc3_Init(void);
u16  Get_Adc3(u8 ch);
#endif
