/**
 * usart_debug.h — USART1 debug serial with RX interrupt (Exp4)
 *
 * USART1 is already initialized by usart_init() for printf TX.
 * This module adds RX interrupt with ring buffer for echo experiment.
 */

#ifndef __USART_DEBUG_H
#define __USART_DEBUG_H

#include "stm32f10x.h"

void USART_RX_Init(void);           /* Enable USART1 RXNE interrupt + NVIC */
uint8_t USART_RX_Available(void);   /* Returns: number of bytes in buffer */
uint8_t USART_RX_Read(void);        /* Read one byte from buffer */
void USART_RX_Flush(void);          /* Clear RX buffer */
void USART_RX_DeInit(void);         /* Disable RXNE interrupt */

#endif /* __USART_DEBUG_H */
