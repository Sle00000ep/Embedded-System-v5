/**
 * tlc5615.h — TLC5615 10-bit DAC driver header (Exp14)
 *
 * GPIO bit-bang (MSB first):
 *   PB12 = CS   (P3)  chip select, low during 16-bit transfer
 *   PB13 = SCLK (P2)  serial clock
 *   PB15 = DIN  (P4)  serial data
 *
 * 16-bit protocol: CS low → send 16 bits MSB first → CS high → update
 * Data format: [4 don't care][10-bit data][2 sub-LSB]
 *   value << 2 → bits D11-D2
 * VREF = 3.3V → output range 0 ~ 6.6V (2×VREF)
 */

#ifndef __TLC5615_H
#define __TLC5615_H

#include "stm32f10x.h"

void TLC5615_Init(void);
void TLC5615_SetValue(uint16_t val);   /* 0-1023 (10-bit) */
void TLC5615_SetVoltage(uint16_t mv);  /* 0-6600 mV (2×VREF=6.6V) */

#endif /* __TLC5615_H */
