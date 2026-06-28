/**
 * rs232.h — USART3 RS232 communication header (Exp15)
 *
 * PB10 = TX (AF_PP), PB11 = RX (IN_FLOATING)
 * USART3 on APB1 (36MHz max)
 */

#ifndef __RS232_H
#define __RS232_H

#include "stm32f10x.h"

void RS232_Init(uint32_t baud);
void RS232_SendByte(uint8_t ch);
void RS232_SendString(char *str);
uint8_t RS232_RxAvailable(void);
uint8_t RS232_ReadByte(void);

#endif /* __RS232_H */
