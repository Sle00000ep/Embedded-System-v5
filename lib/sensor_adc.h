/**
 * sensor_adc.h — ADC1 driver for light sensor & thermistor (Exp3, Exp12)
 *
 * PA1 = ADC123_IN1  = core-board light sensor   (AD Switch must be ON)
 * PC1 = ADC123_IN11 = MainBoard thermistor       (needs jumper: PC1->P120)
 */

#ifndef __SENSOR_ADC_H
#define __SENSOR_ADC_H

#include "stm32f10x.h"

void  ADC1_Init(void);                     /* Init ADC1 with both PA1+PC1 as AIN */
uint16_t ADC1_ReadChannel(uint8_t ch);     /* Single conversion on channel ch */
uint16_t ADC1_ReadAvg(uint8_t ch, uint8_t n); /* Average of n readings */

/* Convenience wrappers */
uint16_t Light_Read(void);                 /* PA1 light sensor, raw 0-4095 */
uint8_t  Light_Percent(void);             /* PA1 light sensor, 0 (dark) - 100 (bright) */
uint16_t Thermistor_Read(void);            /* PC1 thermistor, raw 0-4095 */

#endif /* __SENSOR_ADC_H */
