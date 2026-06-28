/**
 * rs485.h — USART4 RS485 half-duplex communication header (Exp16)
 *
 * PC10 = TX (AF_PP), PC11 = RX (IN_FLOATING), PG7 = DE+RE (direction)
 * MAX3485: DE+/RE- shorted → PG7 high=TX, low=RX
 */

#ifndef __RS485_H
#define __RS485_H

#include "stm32f10x.h"

void RS485_Init(uint32_t baud);
void RS485_SendByte(uint8_t ch);
void RS485_SendString(char *str);
uint8_t RS485_RxAvailable(void);
uint8_t RS485_ReadByte(void);
void RS485_SetTX(void);   /* switch to transmit mode */
void RS485_SetRX(void);   /* switch to receive mode */

#endif /* __RS485_H */
