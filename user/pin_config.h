/**
 * pin_config.h — V5核心板 (STM32F103VET6) 引脚宏定义
 *
 * 宏名与 V4 版保持兼容，内部值替换为 V5 引脚。
 * V4 驱动文件可直接复制到 V5 工程使用，无需修改。
 *
 * V5 vs V4 关键差异:
 *   - 无 PF/PG 端口（VET6 100脚只有 PA-PE）
 *   - 板载 LED 在 PE0-PE15（非 PF0-PF7）
 *   - 大量信号需飞线（PORT 在 V5 上为 NC）
 */

#ifndef __PIN_CONFIG_H
#define __PIN_CONFIG_H

#include "stm32f10x.h"

/* ================================================================
 *  LED1-LED8 (实验1) — GPIOC PC2-PC9 飞线到主板LED1-8 低电平亮
 *  8根飞线: P121→LED1 ... P27→LED8
 *  SW-0 = OFF 关闭核心板PE LED
 * ================================================================ */
#define LED1_PORT       GPIOC
#define LED1_PIN        GPIO_Pin_2
#define LED1_CLK        RCC_APB2Periph_GPIOC

#define LED2_PORT       GPIOC
#define LED2_PIN        GPIO_Pin_3
#define LED2_CLK        RCC_APB2Periph_GPIOC

#define LED3_PORT       GPIOC
#define LED3_PIN        GPIO_Pin_4
#define LED3_CLK        RCC_APB2Periph_GPIOC

#define LED4_PORT       GPIOC
#define LED4_PIN        GPIO_Pin_5
#define LED4_CLK        RCC_APB2Periph_GPIOC

#define LED5_PORT       GPIOC
#define LED5_PIN        GPIO_Pin_6
#define LED5_CLK        RCC_APB2Periph_GPIOC

#define LED6_PORT       GPIOC
#define LED6_PIN        GPIO_Pin_7
#define LED6_CLK        RCC_APB2Periph_GPIOC

#define LED7_PORT       GPIOC
#define LED7_PIN        GPIO_Pin_8
#define LED7_CLK        RCC_APB2Periph_GPIOC

#define LED8_PORT       GPIOC
#define LED8_PIN        GPIO_Pin_9
#define LED8_CLK        RCC_APB2Periph_GPIOC

#define LED_ALL_PORT    GPIOC
#define LED_ALL_PINS    (GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | \
                         GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9)

/* ================================================================
 *  交通灯 (实验5) — GPIOD PD0-PD5 ⚠️全部飞线 低电平亮
 *  NS=North-South, WE=West-East
 *  飞线: P43=NS-R P44=NS-Y P45=NS-G P46=WE-R P47=WE-Y P48=WE-G
 * ================================================================ */
#define NS_R_PORT       GPIOD
#define NS_R_PIN        GPIO_Pin_0
#define NS_R_CLK        RCC_APB2Periph_GPIOD

#define NS_Y_PORT       GPIOD
#define NS_Y_PIN        GPIO_Pin_1
#define NS_Y_CLK        RCC_APB2Periph_GPIOD

#define NS_G_PORT       GPIOD
#define NS_G_PIN        GPIO_Pin_2
#define NS_G_CLK        RCC_APB2Periph_GPIOD

#define WE_R_PORT       GPIOD
#define WE_R_PIN        GPIO_Pin_3
#define WE_R_CLK        RCC_APB2Periph_GPIOD

#define WE_Y_PORT       GPIOD
#define WE_Y_PIN        GPIO_Pin_4
#define WE_Y_CLK        RCC_APB2Periph_GPIOD

#define WE_G_PORT       GPIOD
#define WE_G_PIN        GPIO_Pin_5
#define WE_G_CLK        RCC_APB2Periph_GPIOD

#define TRAFFIC_PORT    GPIOD
#define TRAFFIC_PINS    (GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | \
                         GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5)

/* ================================================================
 *  蜂鸣器 (实验5) — GPIOD PD6 ⚠️飞线 P49→BUZZER 高电平响(8050)
 * ================================================================ */
#define BUZZER_PORT     GPIOD
#define BUZZER_PIN      GPIO_Pin_6
#define BUZZER_CLK      RCC_APB2Periph_GPIOD

/* ================================================================
 *  LCD 12864 串行模式 (实验6)
 *  PSB 硬件接地 → 串口模式. RST/BLA需飞线.
 * ================================================================ */
#define LCD_CS_PORT     GPIOE
#define LCD_CS_PIN      GPIO_Pin_14  /* PE14 原生 P90 */
#define LCD_CS_CLK      RCC_APB2Periph_GPIOE

#define LCD_STD_PORT    GPIOE
#define LCD_STD_PIN     GPIO_Pin_15  /* PE15 原生 P89 */
#define LCD_STD_CLK     RCC_APB2Periph_GPIOE

#define LCD_SCLK_PORT   GPIOB
#define LCD_SCLK_PIN    GPIO_Pin_10  /* PB10 PORT88 ⚠️与RS232_TX共享 */
#define LCD_SCLK_CLK    RCC_APB2Periph_GPIOB

#define LCD_BLA_PORT    GPIOB
#define LCD_BLA_PIN     GPIO_Pin_14  /* PB14 ⚠️飞线 P12→LCD-BLA */
#define LCD_BLA_CLK     RCC_APB2Periph_GPIOB

#define LCD_RST_PORT    GPIOB
#define LCD_RST_PIN     GPIO_Pin_11  /* PB11 PORT87 ⚠️与RS232_RX共享 */
#define LCD_RST_CLK     RCC_APB2Periph_GPIOB

/* ================================================================
 *  CH451 数码管+键盘 (实验7) — GPIOD GPIO模拟4线时序 ⚠️全部飞线
 *  飞线: DIN→P18(PD12) DOUT→P21(PD13)
 * ================================================================ */
#define CH451_DCLK_PORT GPIOD
#define CH451_DCLK_PIN  GPIO_Pin_8   /* PD8 飞线 P14→CH451-DCLK */
#define CH451_DCLK_CLK  RCC_APB2Periph_GPIOD

#define CH451_DIN_PORT  GPIOD
#define CH451_DIN_PIN   GPIO_Pin_9   /* PD9 原生 P17→CH451-DIN (同V4) */
#define CH451_DIN_CLK   RCC_APB2Periph_GPIOD

#define CH451_DOUT_PORT GPIOD
#define CH451_DOUT_PIN  GPIO_Pin_10  /* PD10 原生 P16→CH451-DOUT (同V4) */
#define CH451_DOUT_CLK  RCC_APB2Periph_GPIOD

#define CH451_LOAD_PORT GPIOD
#define CH451_LOAD_PIN  GPIO_Pin_11  /* PD11 飞线 P19→CH451-LOAD */
#define CH451_LOAD_CLK  RCC_APB2Periph_GPIOD

/* ================================================================
 *  独立按键 IKEY0-3 (实验2,8) — 下降沿EXTI 10K上拉
 *  IKEY0/1 ⚠️飞线
 * ================================================================ */
#define IKEY0_PORT      GPIOA
#define IKEY0_PIN       GPIO_Pin_2   /* PA2 ⚠️飞线 P113→IKEY-0 */
#define IKEY0_CLK       RCC_APB2Periph_GPIOA
#define IKEY0_EXTI_LINE EXTI_Line2
#define IKEY0_EXTI_PORT GPIO_PortSourceGPIOA
#define IKEY0_EXTI_PIN  GPIO_PinSource2
#define IKEY0_IRQ       EXTI2_IRQn

#define IKEY1_PORT      GPIOA
#define IKEY1_PIN       GPIO_Pin_3   /* PA3 ⚠️飞线 P109→IKEY-1 */
#define IKEY1_CLK       RCC_APB2Periph_GPIOA
#define IKEY1_EXTI_LINE EXTI_Line3
#define IKEY1_EXTI_PORT GPIO_PortSourceGPIOA
#define IKEY1_EXTI_PIN  GPIO_PinSource3
#define IKEY1_IRQ       EXTI3_IRQn

#define IKEY2_PORT      GPIOE
#define IKEY2_PIN       GPIO_Pin_2   /* PE2 原生 P137 */
#define IKEY2_CLK       RCC_APB2Periph_GPIOE
#define IKEY2_EXTI_LINE EXTI_Line2
#define IKEY2_EXTI_PORT GPIO_PortSourceGPIOE
#define IKEY2_EXTI_PIN  GPIO_PinSource2
#define IKEY2_IRQ       EXTI2_IRQn

#define IKEY3_PORT      GPIOE
#define IKEY3_PIN       GPIO_Pin_3   /* PE3 原生 P134 */
#define IKEY3_CLK       RCC_APB2Periph_GPIOE
#define IKEY3_EXTI_LINE EXTI_Line3
#define IKEY3_EXTI_PORT GPIO_PortSourceGPIOE
#define IKEY3_EXTI_PIN  GPIO_PinSource3
#define IKEY3_IRQ       EXTI3_IRQn

/* ================================================================
 *  拨码开关 SW0-7 (实验9) — 10K下拉 ON=高
 *  SW-5/6 ⚠️飞线, SW-3/4在PC14/PC15(RTC晶振脚,可作GPIO)
 * ================================================================ */
#define SW0_PORT        GPIOE
#define SW0_PIN         GPIO_Pin_4   /* PE4 原生 P135 */
#define SW0_CLK         RCC_APB2Periph_GPIOE

#define SW1_PORT        GPIOE
#define SW1_PIN         GPIO_Pin_5   /* PE5 原生 P132 */
#define SW1_CLK         RCC_APB2Periph_GPIOE

#define SW2_PORT        GPIOC
#define SW2_PIN         GPIO_Pin_13  /* PC13 原生 P131 */
#define SW2_CLK         RCC_APB2Periph_GPIOC

#define SW3_PORT        GPIOC
#define SW3_PIN         GPIO_Pin_14  /* PC14 原生 P128 (RTC晶振脚) */
#define SW3_CLK         RCC_APB2Periph_GPIOC

#define SW4_PORT        GPIOC
#define SW4_PIN         GPIO_Pin_15  /* PC15 原生 P129 (RTC晶振脚) */
#define SW4_CLK         RCC_APB2Periph_GPIOC

#define SW5_PORT        GPIOB
#define SW5_PIN         GPIO_Pin_0   /* PB0 ⚠️飞线 P100→SW-5 */
#define SW5_CLK         RCC_APB2Periph_GPIOB

#define SW6_PORT        GPIOA
#define SW6_PIN         GPIO_Pin_4   /* PA4 ⚠️飞线 P106→SW-6 */
#define SW6_CLK         RCC_APB2Periph_GPIOA

#define SW7_PORT        GPIOB
#define SW7_PIN         GPIO_Pin_1   /* PB1 原生 P99 */
#define SW7_CLK         RCC_APB2Periph_GPIOB

#define SW_ALL_PORT     GPIOE          /* 注: V5上无法单一端口覆盖全部SW */
#define SW_ALL_PINS     (GPIO_Pin_4 | GPIO_Pin_5)  /* SW0-1在PE */

/* ================================================================
 *  继电器 (实验10) — GPIOD PD9/PD10 高电平吸合 8050驱动
 * ================================================================ */
#define RELAY1_PORT     GPIOD
#define RELAY1_PIN      GPIO_Pin_9   /* PD9 原生 P17 */
#define RELAY1_CLK      RCC_APB2Periph_GPIOD

#define RELAY2_PORT     GPIOD
#define RELAY2_PIN      GPIO_Pin_10  /* PD10 原生 P16 */
#define RELAY2_CLK      RCC_APB2Periph_GPIOD

/* ================================================================
 *  DHT11 温湿度 (实验11) — PA5 One-Wire 10K上拉 ⚠️飞线
 * ================================================================ */
#define DHT11_PORT      GPIOA
#define DHT11_PIN       GPIO_Pin_5   /* PA5 ⚠️飞线 P105→DHT11 */
#define DHT11_CLK       RCC_APB2Periph_GPIOA

/* ================================================================
 *  DS18B20 温度 (实验13) — PE9 One-Wire 4.7K上拉
 * ================================================================ */
#define DS18B20_PORT    GPIOE
#define DS18B20_PIN     GPIO_Pin_9   /* PE9 原生 P95 */
#define DS18B20_CLK     RCC_APB2Periph_GPIOE

/* ================================================================
 *  TLC5615 DA (实验14) — SPI2 ⚠️全部飞线
 * ================================================================ */
#define DA_CS_PORT      GPIOB
#define DA_CS_PIN       GPIO_Pin_12  /* PB12 ⚠️飞线 P10→DA-CS */
#define DA_CS_CLK       RCC_APB2Periph_GPIOB

#define DA_CLK_PORT     GPIOB
#define DA_CLK_PIN      GPIO_Pin_13  /* PB13 ⚠️飞线 P13→DA-CLK */
#define DA_CLK_CLK      RCC_APB2Periph_GPIOB

#define DA_DATA_PORT    GPIOB
#define DA_DATA_PIN     GPIO_Pin_15  /* PB15 ⚠️飞线 P15→DA-DATA */
#define DA_DATA_CLK     RCC_APB2Periph_GPIOB

#define DA_SPI          SPI2
#define DA_SPI_CLK      RCC_APB1Periph_SPI2

/* ================================================================
 *  RS232 (实验15) — USART3 ⚠️全部飞线
 * ================================================================ */
#define RS232_TX_PORT   GPIOB
#define RS232_TX_PIN    GPIO_Pin_10  /* PB10 USART3_TX 飞线 P88→232-TXD */
#define RS232_TX_CLK    RCC_APB2Periph_GPIOB

#define RS232_RX_PORT   GPIOB
#define RS232_RX_PIN    GPIO_Pin_11  /* PB11 USART3_RX 飞线 P87→232-RXD */
#define RS232_RX_CLK    RCC_APB2Periph_GPIOB

#define RS232_USART     USART3
#define RS232_USART_CLK RCC_APB1Periph_USART3

/* ================================================================
 *  RS485 (实验16) — USART4 + PD14方向控制
 * ================================================================ */
#define RS485_TX_PORT   GPIOC
#define RS485_TX_PIN    GPIO_Pin_10  /* PC10 原生 P40 */
#define RS485_TX_CLK    RCC_APB2Periph_GPIOC

#define RS485_RX_PORT   GPIOC
#define RS485_RX_PIN    GPIO_Pin_11  /* PC11 原生 P41 */
#define RS485_RX_CLK    RCC_APB2Periph_GPIOC

#define RS485_EN_PORT   GPIOD
#define RS485_EN_PIN    GPIO_Pin_14  /* PD14 原生 P20 */
#define RS485_EN_CLK    RCC_APB2Periph_GPIOD

#define RS485_USART     UART4
#define RS485_USART_CLK RCC_APB1Periph_UART4

/* ================================================================
 *  红外接收 (实验17) — GPIOC EXTI0 下降沿 ⚠️飞线 PC0
 * ================================================================ */
#define IR_PORT         GPIOC
#define IR_PIN          GPIO_Pin_0   /* PC0 ⚠️飞线 P123→INFRARED */
#define IR_CLK          RCC_APB2Periph_GPIOC
#define IR_EXTI_LINE    EXTI_Line0
#define IR_EXTI_PORT    GPIO_PortSourceGPIOC
#define IR_EXTI_PIN     GPIO_PinSource0
#define IR_IRQ          EXTI0_IRQn

/* ================================================================
 *  步进电机 (实验18) — GPIOE ⚠️全部飞线 ULN2003
 * ================================================================ */
#define STEP_A_PORT     GPIOE
#define STEP_A_PIN      GPIO_Pin_12  /* PE12 飞线 P92→PHASE-A */
#define STEP_A_CLK      RCC_APB2Periph_GPIOE

#define STEP_B_PORT     GPIOE
#define STEP_B_PIN      GPIO_Pin_13  /* PE13 飞线 P91→PHASE-B */
#define STEP_B_CLK      RCC_APB2Periph_GPIOE

#define STEP_C_PORT     GPIOE
#define STEP_C_PIN      GPIO_Pin_10  /* PE10 飞线 P94→PHASE-C */
#define STEP_C_CLK      RCC_APB2Periph_GPIOE

#define STEP_D_PORT     GPIOE
#define STEP_D_PIN      GPIO_Pin_11  /* PE11 飞线 P93→PHASE-D */
#define STEP_D_CLK      RCC_APB2Periph_GPIOE

#define STEPPER_PORT    GPIOE
#define STEPPER_PINS    (GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13)

/* ================================================================
 *  编码器 (实验18) — PB6/PB7 TIM4_CH1/CH2 + PD15 Z相
 *  A/B相⚠️飞线, Z相原生
 * ================================================================ */
#define ENC_A_PORT      GPIOB
#define ENC_A_PIN       GPIO_Pin_6   /* PB6 TIM4_CH1 ⚠️飞线 P54→ENCODER-A */
#define ENC_A_CLK       RCC_APB2Periph_GPIOB

#define ENC_B_PORT      GPIOB
#define ENC_B_PIN       GPIO_Pin_7   /* PB7 TIM4_CH2 ⚠️飞线 P55→ENCODER-B */
#define ENC_B_CLK       RCC_APB2Periph_GPIOB

#define ENC_Z_PORT      GPIOD
#define ENC_Z_PIN       GPIO_Pin_15  /* PD15 原生 P23 */
#define ENC_Z_CLK       RCC_APB2Periph_GPIOD

/* ================================================================
 *  直流电机 PWM (实验19) — PA6/PA7 TIM3_CH1/CH2 ⚠️飞线
 * ================================================================ */
#define PWMA_PORT       GPIOA
#define PWMA_PIN        GPIO_Pin_6   /* PA6 TIM3_CH1 ⚠️飞线 P104→PWMA */
#define PWMA_CLK        RCC_APB2Periph_GPIOA

#define PWMB_PORT       GPIOA
#define PWMB_PIN        GPIO_Pin_7   /* PA7 TIM3_CH2 ⚠️飞线 P103→PWMB */
#define PWMB_CLK        RCC_APB2Periph_GPIOA

/* ================================================================
 *  舵机 (实验20) — PB9 TIM4_CH4 50Hz PWM ⚠️飞线
 * ================================================================ */
#define SERVO_PORT      GPIOB
#define SERVO_PIN       GPIO_Pin_8   /* PB8 TIM4_CH3 ⚠️飞线 P57→STEERING */
#define SERVO_CLK       RCC_APB2Periph_GPIOB

/* ================================================================
 *  ADC (实验3,12) — PA1核心板光敏 + PC1主板热敏
 *  SW-1 (AD) = ON 启用核心板光敏
 * ================================================================ */
#define ADC_LIGHT_PORT  GPIOA
#define ADC_LIGHT_PIN   GPIO_Pin_1
#define ADC_LIGHT_CLK   RCC_APB2Periph_GPIOA
#define ADC_LIGHT_CH    ADC_Channel_1       /* ADC123_IN1 */

#define ADC_TEMP_PORT   GPIOC
#define ADC_TEMP_PIN    GPIO_Pin_1
#define ADC_TEMP_CLK    RCC_APB2Periph_GPIOC
#define ADC_TEMP_CH     ADC_Channel_11      /* ADC123_IN11 */

/* ================================================================
 *  调试串口 (实验4) — USART1 PA9/PA10 V5板载CH340G
 * ================================================================ */
#define DBG_TX_PORT     GPIOA
#define DBG_TX_PIN      GPIO_Pin_9
#define DBG_TX_CLK      RCC_APB2Periph_GPIOA

#define DBG_RX_PORT     GPIOA
#define DBG_RX_PIN      GPIO_Pin_10
#define DBG_RX_CLK      RCC_APB2Periph_GPIOA

#define DBG_USART       USART1
#define DBG_USART_CLK   RCC_APB2Periph_USART1

/* ================================================================
 *  WKUP 模式切换按键 (实验2,21) — PA0 V5板载
 * ================================================================ */
#define WKUP_PORT       GPIOA
#define WKUP_PIN        GPIO_Pin_0
#define WKUP_CLK        RCC_APB2Periph_GPIOA

/* ================================================================
 *  定时器
 * ================================================================ */
/* TIM3: 系统时基(0.5s Update IRQ) + DC电机PWM(CH1/CH2) */
#define TIM3_SYS        TIM3
#define TIM3_SYS_CLK    RCC_APB1Periph_TIM3
#define TIM3_PWMA_CH    TIM_Channel_1   /* PA6=TIM3_CH1 */
#define TIM3_PWMB_CH    TIM_Channel_2   /* PA7=TIM3_CH2 */

/* TIM4: 编码器捕获(CH1/CH2) + 舵机PWM(CH3=PB8) */
#define TIM4_SYS        TIM4
#define TIM4_SYS_CLK    RCC_APB1Periph_TIM4
#define TIM4_SERVO_CH   TIM_Channel_3   /* PB8=TIM4_CH3 */
#define TIM4_ENC_A_CH   TIM_Channel_1   /* PB6=TIM4_CH1 */
#define TIM4_ENC_B_CH   TIM_Channel_2   /* PB7=TIM4_CH2 */

#endif /* __PIN_CONFIG_H */
