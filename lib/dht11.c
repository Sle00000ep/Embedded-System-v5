/**
 * dht11.c — DHT11 One-Wire driver (Exp11)
 *
 * PF12, 10K pull-up to VDD.  Interrupts are disabled during the read
 * sequence to protect the microsecond-level timing.
 *
 * Protocol:
 *   MCU: low 18ms → high 20-40us → release
 *   DHT11: low 80us → high 80us → 40 data bits
 *   Bit 0: low 50us + high 26-28us
 *   Bit 1: low 50us + high 70us
 */

#include "dht11.h"
#include "pin_config.h"
#include "delay_us.h"

/* ---- Pin direction helpers ---- */
static void DHT11_PinOut(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

static void DHT11_PinIn(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin  = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

static uint8_t DHT11_ReadPin(void)
{
    return (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == Bit_RESET) ? 0 : 1;
}

static void DHT11_WritePin(uint8_t val)
{
    if (val)
        GPIO_SetBits(DHT11_PORT, DHT11_PIN);
    else
        GPIO_ResetBits(DHT11_PORT, DHT11_PIN);
}

/* ---- Init ---- */
void DHT11_Init(void)
{
    RCC_APB2PeriphClockCmd(DHT11_CLK, ENABLE);
    /* Default: output high (idle) */
    DHT11_PinOut();
    DHT11_WritePin(1);
}

/* ---- Read one byte (8 bits, MSB first) ---- */
static uint8_t DHT11_ReadByte(void)
{
    uint8_t i, val = 0;
    for (i = 0; i < 8; i++) {
        /* Wait for pin to go high (start of data bit) */
        uint32_t timeout = 1000;
        while (!DHT11_ReadPin()) {
            if (--timeout == 0) return 0;
        }

        /* Wait 40us then sample */
        delay_us(40);
        val <<= 1;
        if (DHT11_ReadPin()) val |= 1;

        /* Wait for pin to go low (end of bit) */
        timeout = 1000;
        while (DHT11_ReadPin()) {
            if (--timeout == 0) return 0;
        }
    }
    return val;
}

/* ---- Main read ---- */
DHT11_Data DHT11_Read(void)
{
    DHT11_Data data = {0, 0, DHT11_OK};
    uint8_t buf[5];
    uint32_t timeout;
    uint8_t i;

    __disable_irq();

    /* Step 1: Start signal — low 18ms, high 20-40us */
    DHT11_PinOut();
    DHT11_WritePin(0);
    delay_us(18000);  /* 18ms */
    DHT11_WritePin(1);
    delay_us(30);     /* 30us */

    /* Step 2: Switch to input, wait for DHT11 response */
    DHT11_PinIn();

    /* DHT11 should pull low ~80us */
    timeout = 2000;
    while (DHT11_ReadPin()) {   /* wait for low */
        if (--timeout == 0) { data.status = DHT11_TIMEOUT; goto exit; }
    }
    timeout = 2000;
    while (!DHT11_ReadPin()) {  /* wait for high */
        if (--timeout == 0) { data.status = DHT11_TIMEOUT; goto exit; }
    }
    timeout = 2000;
    while (DHT11_ReadPin()) {   /* wait for low (end of response) */
        if (--timeout == 0) { data.status = DHT11_TIMEOUT; goto exit; }
    }

    /* Step 3: Read 5 bytes */
    for (i = 0; i < 5; i++) {
        buf[i] = DHT11_ReadByte();
    }

    __enable_irq();

    /* Step 4: Verify checksum */
    if (buf[4] != (uint8_t)(buf[0] + buf[1] + buf[2] + buf[3])) {
        data.status = DHT11_CHKSUM;
        return data;
    }

    data.humidity    = buf[0];
    data.temperature = buf[2];
    return data;

exit:
    __enable_irq();
    return data;
}
