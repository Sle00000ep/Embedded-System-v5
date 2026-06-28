/**
 * tlc5615.c — TLC5615 10-bit DAC GPIO bit-bang driver (Exp14)
 *
 * PB12 = CS   (P3)  chip select, low during transfer
 * PB13 = SCLK (P2)  serial clock
 * PB15 = DIN  (P4)  serial data input
 *
 * TLC5615 protocol (MSB first):
 *   - 16-bit shift register
 *   - D15-D12 = don't care (0)
 *   - D11-D2  = 10-bit DAC code
 *   - D1-D0   = sub-LSB (0)
 *   - CS low → clock 16 bits → CS high → DAC output updates
 *
 * VREF = 3.3V, DAC output = 2 × VREF × CODE / 1024 (0 ~ 6.6V)
 */

#include "tlc5615.h"
#include "pin_config.h"
#include "delay_us.h"

/* ------------------------------------------------------------------
 *  Send 16 bits to TLC5615, MSB first
 * ------------------------------------------------------------------ */
static void TLC5615_Send(uint16_t word)
{
    uint8_t i;

    /* CS low → start frame */
    GPIO_ResetBits(DA_CS_PORT, DA_CS_PIN);

    for (i = 0; i < 16; i++) {
        /* Set DIN to bit15 (MSB first) */
        if (word & 0x8000)
            GPIO_SetBits(DA_DATA_PORT, DA_DATA_PIN);
        else
            GPIO_ResetBits(DA_DATA_PORT, DA_DATA_PIN);

        word <<= 1;

        /* Rising clock edge → TLC5615 samples DIN */
        GPIO_ResetBits(DA_CLK_PORT, DA_CLK_PIN);
        delay_us(1);
        GPIO_SetBits(DA_CLK_PORT, DA_CLK_PIN);
        delay_us(1);
    }

    /* CS high → latch → update analog output */
    GPIO_SetBits(DA_CS_PORT, DA_CS_PIN);
    delay_us(1);
}

/* ------------------------------------------------------------------
 *  Init
 * ------------------------------------------------------------------ */
void TLC5615_Init(void)
{
    GPIO_InitTypeDef s;

    RCC_APB2PeriphClockCmd(DA_CS_CLK, ENABLE);   /* GPIOB */

    s.GPIO_Pin   = DA_CS_PIN | DA_CLK_PIN | DA_DATA_PIN;
    s.GPIO_Speed = GPIO_Speed_50MHz;
    s.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(DA_CS_PORT, &s);

    /* Idle: CS high, SCLK low */
    GPIO_SetBits(DA_CS_PORT, DA_CS_PIN);
    GPIO_ResetBits(DA_CLK_PORT, DA_CLK_PIN);

    /* Output 0V initially */
    TLC5615_Send(0x0000);
}

/* ------------------------------------------------------------------
 *  Set DAC with raw 10-bit code (0-1023)
 * ------------------------------------------------------------------ */
void TLC5615_SetValue(uint16_t val)
{
    uint16_t word;

    if (val > 1023) val = 1023;

    /*
     * TLC5615 data format (16 bits):
     *   D15-D12 = 0 (don't care)
     *   D11-D2  = 10-bit DAC data
     *   D1-D0   = 0 (sub-LSB, don't care)
     */
    word = (val & 0x3FF) << 2;   /* bits 11-2 = 10-bit data */
    TLC5615_Send(word);
}

/* ------------------------------------------------------------------
 *  Set output voltage in millivolts (0-6600 mV)
 *  DAC output = 2 × VREF × CODE / 1024
 *  With VREF=3.3V, full scale = 6.6V
 * ------------------------------------------------------------------ */
void TLC5615_SetVoltage(uint16_t mv)
{
    uint32_t code;

    /* Nominal full-scale = 2*VREF = 6.6V.
     * Actual measured full-scale on V5 mainboard ≈ 2.06V.
     * Compensate by scaling target voltage up by 6600/2060 ≈ 3.2x */
    if (mv > 2060) mv = 2060;

    code = ((uint32_t)mv * 1024UL) / 2060UL;
    if (code > 1023) code = 1023;

    TLC5615_SetValue((uint16_t)code);
}
