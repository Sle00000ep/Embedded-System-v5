/**
 * ds18b20.c — DS18B20 One-Wire temperature sensor driver (Exp13)
 *
 * PF13, 4.7K pull-up to VDD.  Interrupts disabled during timing-critical
 * read/write slots (~60us each).
 *
 * Commands used:
 *   0xCC — Skip ROM
 *   0x44 — Convert T (up to 750ms for 12-bit)
 *   0xBE — Read Scratchpad (9 bytes, temp in bytes 0-1)
 */

#include "ds18b20.h"
#include "pin_config.h"
#include "delay_us.h"

/* ---- Pin direction helpers ---- */
static void DS18B20_PinOut(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = DS18B20_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS18B20_PORT, &GPIO_InitStructure);
}

static void DS18B20_PinIn(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin  = DS18B20_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DS18B20_PORT, &GPIO_InitStructure);
}

static uint8_t DS18B20_ReadPin(void)
{
    return (GPIO_ReadInputDataBit(DS18B20_PORT, DS18B20_PIN) == Bit_RESET) ? 0 : 1;
}

static void DS18B20_WritePin(uint8_t val)
{
    if (val)
        GPIO_SetBits(DS18B20_PORT, DS18B20_PIN);
    else
        GPIO_ResetBits(DS18B20_PORT, DS18B20_PIN);
}

/* ---- Init ---- */
void DS18B20_Init(void)
{
    RCC_APB2PeriphClockCmd(DS18B20_CLK, ENABLE);
    DS18B20_PinOut();
    DS18B20_WritePin(1);
}

/* ---- Reset + presence pulse ---- */
uint8_t DS18B20_Reset(void)
{
    uint8_t presence;
    uint32_t timeout;

    DS18B20_PinOut();
    DS18B20_WritePin(0);
    delay_us(480);          /* pull low 480us */

    DS18B20_WritePin(1);    /* release */
    delay_us(60);           /* wait for presence */

    DS18B20_PinIn();
    presence = DS18B20_ReadPin();  /* 0 = device present */

    /* Wait for presence pulse to end */
    timeout = 1000;
    while (!DS18B20_ReadPin()) {
        if (--timeout == 0) break;
    }

    return presence;  /* 0=ok, 1=no device */
}

/* ---- Write one bit ---- */
static void DS18B20_WriteBit(uint8_t bit)
{
    DS18B20_PinOut();
    DS18B20_WritePin(0);
    delay_us(2);  /* hold low */

    if (bit) {
        DS18B20_WritePin(1);  /* release (write 1) */
        delay_us(60);
    } else {
        delay_us(60);          /* hold low (write 0) */
        DS18B20_WritePin(1);
    }
    delay_us(2);  /* recovery */
}

/* ---- Write one byte (LSB first) ---- */
void DS18B20_WriteByte(uint8_t dat)
{
    uint8_t i;
    __disable_irq();
    for (i = 0; i < 8; i++) {
        DS18B20_WriteBit(dat & 0x01);
        dat >>= 1;
    }
    __enable_irq();
}

/* ---- Read one bit ---- */
static uint8_t DS18B20_ReadBit(void)
{
    uint8_t bit;
    DS18B20_PinOut();
    DS18B20_WritePin(0);
    delay_us(2);            /* start read slot */
    DS18B20_WritePin(1);    /* release */
    delay_us(12);           /* wait for data */
    DS18B20_PinIn();
    bit = DS18B20_ReadPin();
    delay_us(50);           /* finish time slot */
    return bit;
}

/* ---- Read one byte (LSB first) ---- */
uint8_t DS18B20_ReadByte(void)
{
    uint8_t i, dat = 0;
    __disable_irq();
    for (i = 0; i < 8; i++) {
        if (DS18B20_ReadBit()) dat |= (1 << i);
    }
    __enable_irq();
    return dat;
}

/* ---- Start temperature conversion ---- */
void DS18B20_StartConvert(void)
{
    if (DS18B20_Reset() == 0) {
        DS18B20_WriteByte(0xCC);  /* Skip ROM */
        DS18B20_WriteByte(0x44);  /* Convert T */
    }
}

/* ---- Read temperature (assumes conversion already done) ---- */
DS18B20_Data DS18B20_ReadTemp(void)
{
    DS18B20_Data result = {0, DS18B20_OK};
    uint8_t lsb, msb;
    int16_t raw;

    if (DS18B20_Reset() != 0) {
        result.status = DS18B20_TIMEOUT;
        return result;
    }

    DS18B20_WriteByte(0xCC);  /* Skip ROM */
    DS18B20_WriteByte(0xBE);  /* Read Scratchpad */

    lsb = DS18B20_ReadByte();
    msb = DS18B20_ReadByte();

    raw = (int16_t)((uint16_t)msb << 8) | lsb;
    /* Default 12-bit resolution: temp = raw * 0.0625 C */
    /* Multiply by 10 for one decimal place: raw * 0.625  = raw * 5 / 8 */
    result.temp_x10 = (int16_t)(((int32_t)raw * 10) / 16);
    /* More precisely: ((int32_t)raw * 625 + 500) / 1000  but the above is fine */
    /* Actually: raw * 0.0625 * 10 = raw * 5 / 8 */
    result.temp_x10 = (raw * 5) / 8;

    return result;
}
