/**
 * sensor_adc.c — ADC1 driver (Exp3 light sensor, Exp12 thermistor)
 *
 * PA1 = ADC123_IN1  (ADC_Channel_1)  — core-board photoresistor
 * PC1 = ADC123_IN11 (ADC_Channel_11) — MainBoard NTC thermistor
 *
 * ADC1, independent mode, single conversion, software trigger.
 */

#include "sensor_adc.h"
#include "pin_config.h"

/**
 * Initialize PA1 + PC1 as analog inputs and configure ADC1.
 */
void ADC1_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    ADC_InitTypeDef   ADC_InitStructure;

    /* GPIO clocks */
    RCC_APB2PeriphClockCmd(ADC_LIGHT_CLK, ENABLE);  /* GPIOA */
    RCC_APB2PeriphClockCmd(ADC_TEMP_CLK,  ENABLE);  /* GPIOC */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    /* PA1 — analog input (light sensor) */
    GPIO_InitStructure.GPIO_Pin  = ADC_LIGHT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(ADC_LIGHT_PORT, &GPIO_InitStructure);

    /* PC1 — analog input (thermistor) */
    GPIO_InitStructure.GPIO_Pin  = ADC_TEMP_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(ADC_TEMP_PORT, &GPIO_InitStructure);

    /* ADC1 reset + config */
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, DISABLE);

    ADC_DeInit(ADC1);

    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode       = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel       = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);

    /* Calibration */
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));
}

/**
 * Single ADC1 conversion on a given channel.
 * ch:  ADC_Channel_1 (PA1, light) or ADC_Channel_11 (PC1, thermistor)
 */
uint16_t ADC1_ReadChannel(uint8_t ch)
{
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    return (uint16_t)ADC_GetConversionValue(ADC1);
}

/**
 * Average of n readings (simple filter).
 */
uint16_t ADC1_ReadAvg(uint8_t ch, uint8_t n)
{
    uint32_t sum = 0;
    uint8_t i;
    for (i = 0; i < n; i++) {
        sum += ADC1_ReadChannel(ch);
    }
    return (uint16_t)(sum / n);
}

/* ---- Convenience wrappers ---- */

uint16_t Light_Read(void)
{
    return ADC1_ReadChannel(ADC_LIGHT_CH);  /* ADC_Channel_1 */
}

/**
 * Light sensor reading as percentage (0 = darkest, 100 = brightest).
 * Core-board photoresistor: higher ADC = brighter (voltage divider with
 * fixed resistor to VDD; photoresistor to GND → more light = lower R = higher ADC).
 * If connected opposite, invert: return 100 - pct;
 */
uint8_t Light_Percent(void)
{
    uint16_t val = ADC1_ReadAvg(ADC_LIGHT_CH, 8);
    /* 0-4095 -> 0-100.
     * Core-board photoresistor divider: more light -> lower R -> HIGHER ADC.
     * User observed: dark=high%%, light=low%%. Invert: brightness = 100 - raw%%. */
    uint32_t pct = ((uint32_t)val * 100) / 4095;
    return (uint8_t)(100 - pct);
}

uint16_t Thermistor_Read(void)
{
    return ADC1_ReadChannel(ADC_TEMP_CH);  /* ADC_Channel_11 */
}
