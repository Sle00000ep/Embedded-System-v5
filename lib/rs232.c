/**
 * rs232.c — USART3 RS232 communication driver (Exp15)
 *
 * USART3 fully remapped to PD8(TX) / PD9(RX) to avoid PB10/PB11 conflict with LCD.
 * Original: PB10=USART3_TX(P88), PB11=USART3_RX(P87) shared with LCD SCLK/RST.
 * Remapped: PD8=USART3_TX,      PD9=USART3_RX      (free during Exp15).
 *
 * Fly wires: PD8→RS232-TXD, PD9→RS232-RXD (mainboard RS232 module TTL side).
 *
 * For loopback test without RS232 adapter:
 *   Short PD8—PD9 on core board, or short DB9 pins 2-3 on MainBoard.
 *
 * Ring buffer for RX; TX is blocking (simple).
 */

#include "rs232.h"
#include "pin_config.h"

#define RS232_RX_BUF_SIZE  256

static volatile uint8_t rx_buf[RS232_RX_BUF_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;
static volatile uint8_t rx_count = 0;

void RS232_Init(uint32_t baud)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Clocks — GPIOD for remapped PD8/PD9, AFIO for remap */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RS232_USART_CLK, ENABLE); /* USART3 */

    /* USART3 full remap: PB10/PB11 → PD8/PD9 (free, no LCD conflict) */
    GPIO_PinRemapConfig(GPIO_FullRemap_USART3, ENABLE);

    /* PD8 = TX (alternate function push-pull) */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    /* PD9 = RX (input floating) */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    /* USART3 config */
    USART_InitStructure.USART_BaudRate            = baud;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(RS232_USART, &USART_InitStructure);

    /* Enable RXNE interrupt */
    USART_ITConfig(RS232_USART, USART_IT_RXNE, ENABLE);

    /* NVIC */
    NVIC_InitStructure.NVIC_IRQChannel                   = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority  = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority         = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                 = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Enable USART3 */
    USART_Cmd(RS232_USART, ENABLE);

    /* Flush buffers */
    rx_head = rx_tail = rx_count = 0;
}

void RS232_SendByte(uint8_t ch)
{
    while (USART_GetFlagStatus(RS232_USART, USART_FLAG_TXE) == RESET);
    USART_SendData(RS232_USART, ch);
}

void RS232_SendString(char *str)
{
    while (*str) {
        RS232_SendByte((uint8_t)*str);
        str++;
    }
}

uint8_t RS232_RxAvailable(void)
{
    return rx_count;
}

uint8_t RS232_ReadByte(void)
{
    uint8_t ch;
    if (rx_count == 0) return 0;

    ch = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % RS232_RX_BUF_SIZE;
    rx_count--;
    return ch;
}

/**
 * USART3 interrupt handler — RXNE stores byte in ring buffer
 */
void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(RS232_USART, USART_IT_RXNE) != RESET) {
        uint8_t ch = (uint8_t)USART_ReceiveData(RS232_USART);

        if (rx_count < RS232_RX_BUF_SIZE) {
            rx_buf[rx_head] = ch;
            rx_head = (rx_head + 1) % RS232_RX_BUF_SIZE;
            rx_count++;
        }
    }
}
