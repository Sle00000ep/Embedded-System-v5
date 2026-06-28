/**
 * usart_debug.c — USART1 RX interrupt + ring buffer (Exp4)
 *
 * usart_init() must be called BEFORE usart_rx_init().
 * ISR captures incoming bytes into a 256-byte ring buffer.
 */

#include "usart_debug.h"

#define RX_BUF_SIZE    256

static volatile uint8_t rx_buf[RX_BUF_SIZE];
static volatile uint8_t rx_head = 0;   /* ISR writes here */
static volatile uint8_t rx_tail = 0;   /* main loop reads here */
static volatile uint8_t rx_count = 0;

/**
 * Enable USART1 RXNE interrupt.
 * Call AFTER usart_init() has set up PA9/PA10 and USART1.
 */
void USART_RX_Init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable RXNE (receive data register not empty) interrupt */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    /* NVIC: priority 0 (highest) for debug serial responsiveness */
    NVIC_InitStructure.NVIC_IRQChannel                   = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority  = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority         = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                 = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/**
 * Get number of unread bytes in ring buffer.
 */
uint8_t USART_RX_Available(void)
{
    return rx_count;
}

/**
 * Read one byte from ring buffer.
 * Call only when USART_RX_Available() > 0.
 * Returns 0 if buffer is empty.
 */
uint8_t USART_RX_Read(void)
{
    uint8_t ch;
    if (rx_count == 0) return 0;

    ch = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
    rx_count--;
    return ch;
}

/**
 * Discard all buffered data.
 */
void USART_RX_Flush(void)
{
    rx_head  = 0;
    rx_tail  = 0;
    rx_count = 0;
}

/**
 * Disable USART1 RXNE interrupt (call when leaving Exp4).
 */
void USART_RX_DeInit(void)
{
    USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
}

/**
 * USART1 interrupt handler.
 * RXNE: store incoming byte in ring buffer.
 */
void USART1_IRQHandler(void)
{
    /* RXNE — data received */
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t ch = (uint8_t)USART_ReceiveData(USART1);

        if (rx_count < RX_BUF_SIZE) {
            rx_buf[rx_head] = ch;
            rx_head = (rx_head + 1) % RX_BUF_SIZE;
            rx_count++;
        }
        /* If buffer full, byte is dropped (shouldn't happen in this app) */
    }

    /* Other USART1 interrupts (TXE, TC, etc.) — ignored */
}
