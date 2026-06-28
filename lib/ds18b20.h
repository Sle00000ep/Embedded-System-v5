/**
 * ds18b20.h — DS18B20 temperature sensor header (Exp13)
 *
 * PF13 = One-Wire data line, 4.7K external pull-up required.
 */

#ifndef __DS18B20_H
#define __DS18B20_H

#include "stm32f10x.h"

#define DS18B20_OK      0
#define DS18B20_TIMEOUT 1

typedef struct {
    int16_t temp_x10;   /* temperature x10 (e.g. 256 = 25.6 C) */
    uint8_t status;     /* DS18B20_OK / DS18B20_TIMEOUT */
} DS18B20_Data;

void DS18B20_Init(void);
uint8_t DS18B20_Reset(void);        /* return 0=ok, 1=no device */
void DS18B20_WriteByte(uint8_t dat);
uint8_t DS18B20_ReadByte(void);
void DS18B20_StartConvert(void);    /* issue Convert T command */
DS18B20_Data DS18B20_ReadTemp(void); /* read scratchpad + convert */

#endif /* __DS18B20_H */
