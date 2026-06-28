/**
 * dht11.h — DHT11 temperature & humidity sensor header (Exp11)
 *
 * PF12 = One-Wire data line, 10K external pull-up required.
 */

#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

/* Return status */
#define DHT11_OK      0
#define DHT11_TIMEOUT 1
#define DHT11_CHKSUM  2

typedef struct {
    uint8_t humidity;    /* 0-90 %RH (integer part) */
    uint8_t temperature; /* 0-50 C (integer part) */
    uint8_t status;      /* DHT11_OK / DHT11_TIMEOUT / DHT11_CHKSUM */
} DHT11_Data;

void DHT11_Init(void);
DHT11_Data DHT11_Read(void);

#endif /* __DHT11_H */
