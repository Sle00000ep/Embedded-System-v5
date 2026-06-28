/**
 * lcd12864.c — ST7920 LCD 12864 serial mode driver (Exp6)
 *
 * PE14 = CS  (P90)  chip select, active high   (原生)
 * PE15 = STD (P89)  serial data SID             (原生)
 * PB10 = SCLK (P88)  serial clock  ⚠️与RS232_TX共享(软件仲裁)
 * PB14 = BLA (P12)  backlight, high=on          (飞线)
 * PB11 = RST  (P87)  reset, active low  ⚠️与RS232_RX共享(软件仲裁)
 *
 * ST7920 serial protocol (PSB=GND):
 *   Sync byte: 0xF8 = write command (RS=0,RW=0)
 *              0xFA = write data    (RS=1,RW=0)
 *   Each data byte sent as two 4-bit nibbles (high then low).
 */

#include "lcd12864.h"
#include "pin_config.h"
#include "delay.h"
#include "delay_us.h"

/* ------------------------------------------------------------------
 *  Low-level serial output
 * ------------------------------------------------------------------ */

static void LCD12864_SendByte(uint8_t byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (byte & 0x80)
            GPIO_SetBits(LCD_STD_PORT, LCD_STD_PIN);
        else
            GPIO_ResetBits(LCD_STD_PORT, LCD_STD_PIN);

        GPIO_SetBits(LCD_SCLK_PORT, LCD_SCLK_PIN);
        byte <<= 1;
        GPIO_ResetBits(LCD_SCLK_PORT, LCD_SCLK_PIN);
    }
}

static void LCD12864_WriteCmd(uint8_t cmd)
{
    GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN);       /* CS=1 enable */

    LCD12864_SendByte(0xF8);                     /* sync: command write */
    LCD12864_SendByte(cmd & 0xF0);               /* high nibble */
    LCD12864_SendByte((cmd << 4) & 0xF0);        /* low nibble */

    GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN);     /* CS=0 */
    delay_us(72);                                /* command execution time */
}

static void LCD12864_WriteData(uint8_t data)
{
    GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN);       /* CS=1 enable */

    LCD12864_SendByte(0xFA);                     /* sync: data write */
    LCD12864_SendByte(data & 0xF0);              /* high nibble */
    LCD12864_SendByte((data << 4) & 0xF0);       /* low nibble */

    GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN);     /* CS=0 */
    delay_us(72);
}

/* ------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------ */

void LCD12864_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Clock */
    RCC_APB2PeriphClockCmd(LCD_CS_CLK  |
                           LCD_STD_CLK  |
                           LCD_SCLK_CLK |
                           LCD_BLA_CLK  |
                           LCD_RST_CLK, ENABLE);

    /* PE14(CS) + PE15(STD) on GPIOE */
    GPIO_InitStructure.GPIO_Pin   = LCD_CS_PIN | LCD_STD_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(LCD_CS_PORT, &GPIO_InitStructure);

    /* PB10(SCLK) + PB11(RST) + PB14(BLA) on GPIOB */
    GPIO_InitStructure.GPIO_Pin   = LCD_SCLK_PIN | LCD_RST_PIN | LCD_BLA_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(LCD_SCLK_PORT, &GPIO_InitStructure);

    /* Default idle: CS=0, SCLK=0, STD=0 */
    GPIO_ResetBits(LCD_CS_PORT,   LCD_CS_PIN);
    GPIO_ResetBits(LCD_SCLK_PORT, LCD_SCLK_PIN);
    GPIO_ResetBits(LCD_STD_PORT,  LCD_STD_PIN);

    /* Backlight on */
    GPIO_SetBits(LCD_BLA_PORT, LCD_BLA_PIN);

    /* Hardware reset pulse (RST low ≥ 10ms then high) */
    GPIO_ResetBits(LCD_RST_PORT, LCD_RST_PIN);
    delay_ms(15);
    GPIO_SetBits(LCD_RST_PORT, LCD_RST_PIN);
    delay_ms(50);                                 /* wait for VDD stable */

    /* ST7920 serial-mode init sequence */
    LCD12864_WriteCmd(0x30);                      /* Function: 8-bit, basic inst */
    delay_ms(5);
    LCD12864_WriteCmd(0x30);                      /* required 2nd time */
    delay_ms(1);
    LCD12864_WriteCmd(0x0C);                      /* Display ON, cursor OFF */
    delay_ms(1);
    LCD12864_WriteCmd(0x01);                      /* Clear display */
    delay_ms(10);
    LCD12864_WriteCmd(0x06);                      /* Entry: increment, no shift */
    delay_ms(1);
}

void LCD12864_Clear(void)
{
    LCD12864_WriteCmd(0x01);
    delay_ms(10);
}

/**
 * Set cursor to (row, col).
 * row 0-3, col 0-15 (16-char per line with 5x8 font in graphics mode)
 *
 * ST7920 DDRAM line addresses:
 *   Row 0: 0x80-0x87
 *   Row 1: 0x90-0x97
 *   Row 2: 0x88-0x8F
 *   Row 3: 0x98-0x9F
 */
void LCD12864_SetPos(uint8_t row, uint8_t col)
{
    static const uint8_t row_base[4] = { 0x80, 0x90, 0x88, 0x98 };
    if (row > 3) row = 3;
    if (col > 15) col = 15;  /* 16 half-width chars per line (128px/8px) */
    LCD12864_WriteCmd(row_base[row] + col);
}

void LCD12864_WriteChar(char ch)
{
    LCD12864_WriteData((uint8_t)ch);
}

void LCD12864_WriteString(const char *str)
{
    while (*str) {
        LCD12864_WriteData((uint8_t)*str++);
    }
}

/**
 * Display signed integer on LCD at current position.
 */
void LCD12864_DisplayNum(int32_t num)
{
    char buf[12];
    int i = 0, j;
    uint8_t neg = 0;

    if (num < 0) { neg = 1; num = -num; }
    if (num == 0) { buf[i++] = '0'; }
    else {
        while (num > 0 && i < 10) {
            buf[i++] = '0' + (num % 10);
            num /= 10;
        }
    }
    if (neg) buf[i++] = '-';

    /* Reverse into place */
    for (j = i - 1; j >= 0; j--)
        LCD12864_WriteData(buf[j]);
}

/**
 * Display float with specified decimal places (0-3).
 */
void LCD12864_DisplayFloat(float val, uint8_t decimals)
{
    int32_t i_val;
    uint8_t i;

    if (decimals > 3) decimals = 3;

    /* Multiply */
    uint32_t mul = 1;
    for (i = 0; i < decimals; i++) mul *= 10;

    i_val = (int32_t)(val * mul);

    if (decimals == 0) {
        LCD12864_DisplayNum(i_val);
        return;
    }

    /* Print integer part with sign */
    int32_t int_part = i_val / (int32_t)mul;
    uint32_t frac_part = (i_val < 0) ? (-i_val) % mul : i_val % mul;

    if (int_part == 0 && i_val < 0) {
        LCD12864_WriteChar('-');
        LCD12864_WriteChar('0');
    } else {
        LCD12864_DisplayNum(int_part);
    }

    LCD12864_WriteChar('.');

    /* Fractional part with leading zeros */
    uint32_t div = mul / 10;
    for (i = 0; i < decimals; i++) {
        LCD12864_WriteChar('0' + (frac_part / div) % 10);
        div /= 10;
    }
}

void LCD12864_Backlight(uint8_t on)
{
    if (on)
        GPIO_SetBits(LCD_BLA_PORT, LCD_BLA_PIN);
    else
        GPIO_ResetBits(LCD_BLA_PORT, LCD_BLA_PIN);
}
