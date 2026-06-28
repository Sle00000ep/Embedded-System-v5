/**
 * rs485.c — USART4 RS485 half-duplex communication driver (Exp16)
 *
 * PC10 = USART4_TX → MainBoard MAX3485 DI
 * PC11 = USART4_RX ← MainBoard MAX3485 RO
 * PG7  = DE + /RE  → direction control (high=TX, low=RX)
 *
 * For loopback test without RS485 adapter:
 *   Jumper PC10—PC11 on core board (PG7 switches between TX/RX modes).
 *
 * Ring buffer for RX; TX blocks until TC then returns to RX mode.
 */

#include "rs485.h"
#include "pin_config.h"

#define RS485_RX_BUF_SIZE  256

static volatile uint8_t rx_buf[RS485_RX_BUF_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;
static volatile uint8_t rx_count = 0;

/**
 * Switch RS485 to transmit mode (PG7 high)
 */
void RS485_SetTX(void)
{
    GPIO_SetBits(RS485_EN_PORT, RS485_EN_PIN);
}

/**
 * Switch RS485 to receive mode (PG7 low, default)
 */
void RS485_SetRX(void)
{
    GPIO_ResetBits(RS485_EN_PORT, RS485_EN_PIN);
}

void RS485_Init(uint32_t baud)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Clocks */
    RCC_APB2PeriphClockCmd(RS485_TX_CLK, ENABLE);  /* GPIOC */
    RCC_APB2PeriphClockCmd(RS485_EN_CLK, ENABLE);  /* GPIOG */
    RCC_APB1PeriphClockCmd(RS485_USART_CLK, ENABLE); /* USART4 */

    /* PC10 = TX */
    GPIO_InitStructure.GPIO_Pin   = RS485_TX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(RS485_TX_PORT, &GPIO_InitStructure);

    /* PC11 = RX */
    GPIO_InitStructure.GPIO_Pin  = RS485_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(RS485_RX_PORT, &GPIO_InitStructure);

    /* PG7 = direction control (push-pull output) */
    GPIO_InitStructure.GPIO_Pin   = RS485_EN_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(RS485_EN_PORT, &GPIO_InitStructure);

    /* Default: receive mode */
    RS485_SetRX();

    /* USART4 (UART4) config */
    USART_InitStructure.USART_BaudRate            = baud;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(RS485_USART, &USART_InitStructure);

    /* Enable RXNE interrupt */
    USART_ITConfig(RS485_USART, USART_IT_RXNE, ENABLE);

    /* NVIC */
    NVIC_InitStructure.NVIC_IRQChannel                   = UART4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority  = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority         = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                 = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Enable USART4 */
    USART_Cmd(RS485_USART, ENABLE);

    /* Flush buffers */
    rx_head = rx_tail = rx_count = 0;
}

/**
 * Send one byte in half-duplex mode:
 *   1. Switch to TX mode
 *   2. Wait for TXE (buffer empty)
 *   3. Send data
 *   4. Wait for TC (shift register done)
 *   5. Switch back to RX mode
 */
void RS485_SendByte(uint8_t ch)
{
    RS485_SetTX();

    /* Small delay for DE to stabilize (RS485 transceiver spec: <100ns typically) */
    for (volatile int i = 0; i < 100; i++) __NOP();

    while (USART_GetFlagStatus(RS485_USART, USART_FLAG_TXE) == RESET);
    USART_SendData(RS485_USART, ch);

    /* Wait until byte has been fully shifted out */
    while (USART_GetFlagStatus(RS485_USART, USART_FLAG_TC) == RESET);

    RS485_SetRX();
}

void RS485_SendString(char *str)
{
    while (*str) {
        RS485_SendByte((uint8_t)*str);
        str++;
    }
}

uint8_t RS485_RxAvailable(void)
{
    return rx_count;
}

uint8_t RS485_ReadByte(void)
{
    uint8_t ch;
    if (rx_count == 0) return 0;

    ch = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % RS485_RX_BUF_SIZE;
    rx_count--;
    return ch;
}

/**
 * USART4 (UART4) interrupt handler — RXNE stores byte in ring buffer
 */
void UART4_IRQHandler(void)
{
    if (USART_GetITStatus(RS485_USART, USART_IT_RXNE) != RESET) {
        uint8_t ch = (uint8_t)USART_ReceiveData(RS485_USART);

        if (rx_count < RS485_RX_BUF_SIZE) {
            rx_buf[rx_head] = ch;
            rx_head = (rx_head + 1) % RS485_RX_BUF_SIZE;
            rx_count++;
        }
    }
}
