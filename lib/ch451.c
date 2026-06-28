/**
 * ch451.c — CH451 数码管+键盘驱动 (Exp7)
 *
 * 数据手册关键协议 (CH451DS1):
 *   1. LSB 优先! (bit0 → bit11)
 *   2. LOAD=0 期间发送12位, LOAD=1 锁存
 *   3. 上电需 DIN 0→1 跳变选通 4 线模式
 *   4. 加载字数据: 1[DIG_ADDR(3)][DIG_DATA(8)]
 *      DIG0=0x0800 ... DIG7=0x0F00
 *   5. 显示参数: 0101[MODE][LIMIT(3)][INTENSITY(4)]
 *      MODE=1=BCD译码, LIMIT=000=8位扫描
 *   6. 按键读取: 7位, bit6=按下标志, bit5-0=键码
 */

#include "ch451.h"
#include "pin_config.h"
#include "delay.h"
#include "delay_us.h"

/* ================================================================
 *  12-bit write — LSB first!
 * ================================================================ */
void CH451_WriteCmd(uint16_t word)
{
    uint8_t i;

    /* LOAD=0 → 命令开始 */
    GPIO_ResetBits(CH451_LOAD_PORT, CH451_LOAD_PIN);

    for (i = 0; i < 12; i++) {
        /* Set DIN to LSB */
        if (word & 0x0001)
            GPIO_SetBits(CH451_DIN_PORT, CH451_DIN_PIN);
        else
            GPIO_ResetBits(CH451_DIN_PORT, CH451_DIN_PIN);

        word >>= 1;

        /* Rising clock edge → CH451 samples DIN */
        GPIO_ResetBits(CH451_DCLK_PORT, CH451_DCLK_PIN);
        delay_us(1);
        GPIO_SetBits(CH451_DCLK_PORT, CH451_DCLK_PIN);
        delay_us(1);
    }

    /* LOAD=1 → 命令锁存生效 */
    GPIO_SetBits(CH451_LOAD_PORT, CH451_LOAD_PIN);
}

/* ================================================================
 *  Init
 * ================================================================ */
void CH451_Init(void)
{
    GPIO_InitTypeDef s;

    RCC_APB2PeriphClockCmd(CH451_DCLK_CLK | CH451_DIN_CLK |
                           CH451_DOUT_CLK | CH451_LOAD_CLK, ENABLE);

    s.GPIO_Pin   = CH451_DCLK_PIN | CH451_DIN_PIN | CH451_LOAD_PIN;
    s.GPIO_Speed = GPIO_Speed_50MHz;
    s.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOD, &s);

    s.GPIO_Pin  = CH451_DOUT_PIN;
    s.GPIO_Mode = GPIO_Mode_IPD;   /* input pull-down (同V4) */
    GPIO_Init(GPIOD, &s);

    /*
     * 上电选通 4 线串行接口:
     *   DIN 先置低再置高 → CH451 识别为 4 线模式
     */
    GPIO_ResetBits(CH451_DIN_PORT, CH451_DIN_PIN);
    delay_us(10);
    GPIO_SetBits(CH451_DIN_PORT, CH451_DIN_PIN);
    delay_us(10);

    GPIO_SetBits(CH451_LOAD_PORT, CH451_LOAD_PIN);
    GPIO_SetBits(CH451_DCLK_PORT, CH451_DCLK_PIN);

    /* 初始化序列 (手册参考) */
    CH451_WriteCmd(0x0201);   /* 内部复位 */
    CH451_WriteCmd(0x0403);   /* 系统: 开显示(b0) + 开键盘(b1) */
    CH451_WriteCmd(0x0580);   /* 显示: BCD译码(b7=1) + 8位扫描 + 亮度16/16 */
    CH451_Clear();
}

/* ================================================================
 *  Display
 * ================================================================ */

/**
 * 写单个数码管 (BCD译码模式)
 *   pos: 0-7
 *   val: 0-9=数字, 0x10=空格, 0x0A-0x0F=字母A-F
 */
void CH451_SetDigit(uint8_t pos, uint8_t val)
{
    if (pos > 7) return;
    /* DIG0=0x0800 ... DIG7=0x0F00, OR with data in lower 8 bits */
    CH451_WriteCmd((uint16_t)((0x08 + pos) << 8) | (val & 0xFF));
}

void CH451_Clear(void)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
        CH451_SetDigit(i, 0x10);   /* 0x10 = 空格 */
}

/**
 * 显示整数, 右对齐, 消前导零
 *
 * CH451 DIG0 对应物理最右位数码管（个位）,
 * buf[0]=个位→DIG0, buf[1]=十位→DIG1, ... 依次左移
 */
void CH451_DisplayNum(int32_t num)
{
    uint8_t buf[8], len = 0, neg = 0, i;

    if (num < -9999999) num = -9999999;
    if (num > 99999999) num = 99999999;

    if (num < 0) { neg = 1; num = -num; }
    if (num == 0) { buf[len++] = 0; }
    else {
        while (num > 0 && len < 8) {
            buf[len++] = num % 10;
            num /= 10;
        }
    }

    /*
     * buf[0]=个位, buf[1]=十位, ... buf[len-1]=最高位
     * DIG0=最右边(个位), DIG1=十位, ...
     * pos=i 直接对应 buf[i]:
     *   右边数码管(DIG0) = 个位
     *   往左依次是十位、百位、千位...
     */
    for (i = 0; i < 8; i++) {
        if (i < len)
            CH451_SetDigit(i, buf[i]);           /* buf[i] → DIG i (右起第 i+1 位) */
        else if (i == len && neg)
            CH451_SetDigit(i, 0x0B);             /* BCD: 'b' 作为负号显示 */
        else
            CH451_SetDigit(i, 0x10);             /* 空格 */
    }
}

void CH451_SetBrightness(uint8_t level)
{
    uint8_t l;
    if (level > 7) level = 7;
    /*
     * 亮度编码: 0000=16/16(最亮), 0001=1/16, ..., 1111=15/16
     * 简单映射: 0→最亮(0), 7→中等(8)
     */
    l = 8 - level;   /* level 0=8/16, level 7=1/16 */
    if (l > 15) l = 15;
    CH451_WriteCmd(0x0580 | (l & 0x0F));  /* BCD译码 + 8位 + 亮度 */
}

/* ================================================================
 *  Keyboard — 7位键码, bit6=按下标志, bit5-0=键号
 * ================================================================ */
uint8_t CH451_ReadKey(void)
{
    uint8_t i, key = 0;

    /* 发送读键命令 (高4位 0111 即可) */
    CH451_WriteCmd(0x0700);
    delay_us(10);

    /*
     * 从 DOUT 读 7 位键码 (手册要求)
     *   bit6 = 状态 (1=按下)
     *   bit5-0 = 键号 (0-63)
     */
    for (i = 0; i < 7; i++) {
        key <<= 1;
        if (GPIO_ReadInputDataBit(CH451_DOUT_PORT, CH451_DOUT_PIN) != Bit_RESET)
            key |= 1;

        GPIO_ResetBits(CH451_DCLK_PORT, CH451_DCLK_PIN);
        delay_us(2);
        GPIO_SetBits(CH451_DCLK_PORT, CH451_DCLK_PIN);
        delay_us(2);
    }

    /* bit6=0 → 按键未按下 / 按键已释放 */
    if ((key & 0x40) == 0)
        return 0xFF;

    return key & 0x3F;   /* bits 5-0 = 键号 */
}

/**
 * 原始段码写入 (不译码模式用)
 *   segments: bit0=a .. bit6=g, bit7=dp
 */
void CH451_SetRaw(uint8_t pos, uint8_t segments)
{
    if (pos > 7) return;
    CH451_WriteCmd((uint16_t)((0x08 + pos) << 8) | segments);
}
