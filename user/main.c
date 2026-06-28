/**
 * main.c — V5 CoreBoard (STM32F103VET6) + MainBoard 21-Function FreeRTOS
 *
 * 2-task architecture:
 *   ControlTask     (prio 4): WKUP polling, mode switching via queue
 *   ExperimentTask  (prio 3): runs current experiment until stop_requested
 *
 * Adapted from V4 (STM32F103ZET6). Key differences:
 *   - No PF/PG ports (VET6 100-pin)
 *   - Onboard LEDs on PE0-PE15 (SW-0=ON)
 *   - Many signals need fly wires (PORT positions NC on V5)
 */

#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "gpio_out.h"
#include "gpio_in.h"
#include "usart_debug.h"
#include "rs232.h"
#include "rs485.h"
#include "sensor_adc.h"
#include "dht11.h"
#include "ds18b20.h"
#include "infrared.h"
#include "timer_pwm.h"
#include "encoder.h"
#include "delay_us.h"
#include "lcd12864.h"
#include "ch451.h"
#include "relay.h"
#include "tlc5615.h"
#include "stepper.h"
#include "pin_config.h"
#include "freertos_tasks.h"

/* ============================================================
 *  WKUP Key (PA0) — used by ControlTask via extern
 * ============================================================ */
uint8_t WKUP_IsPressed(void)
{
    return (GPIO_ReadInputDataBit(WKUP_PORT, WKUP_PIN) == Bit_RESET) ? 1 : 0;
}

void WKUP_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(WKUP_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin   = WKUP_PIN;
    GPIO_Init(WKUP_PORT, &GPIO_InitStructure);
}

/* ============================================================
 *  Exp1: LED Flowing  (PE0-PE7, 200ms/step)
 * ============================================================ */
void Exp1_LED_Flowing(void)
{
    uint8_t i = 0;
    uint32_t lcd_tick = 0;
    char buf[17];
    LED_Init();
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp1: LED Flowing =====\r\n");
    while (1) {
        if (stop_requested) { LED_AllOff(); LCD12864_Clear(); LCD12864_Backlight(0); return; }
        LED_AllOff();
        LED_Set(i, 1);
        i = (i + 1) % 8;

        if (++lcd_tick >= 2) {  /* every 2x200=400ms */
            lcd_tick = 0;
            LCD12864_SetPos(0,0); LCD12864_WriteString("LED Flow");
            LCD12864_SetPos(1,0);
            sprintf(buf,"Pos=%-3d  ",i); LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0);
            { uint8_t j; for(j=0;j<8;j++) buf[j]=(j<=i)?'>':' '; buf[8]=0; LCD12864_WriteString(buf); }
            LCD12864_SetPos(3,0); LCD12864_WriteString("SPD=200 ");
        }
        delay_ms(200);
    }
}

/* ============================================================
 *  Exp5: Traffic Light  (PD0-PD5 + PD6 buzzer)
 * ============================================================ */
typedef enum { TL_NS_G_WE_R, TL_NS_Y_WE_R, TL_NS_R_WE_G, TL_NS_R_WE_Y } TrafficState;
void Exp5_TrafficLight(void)
{
    TrafficState s = TL_NS_G_WE_R;
    uint32_t ticks = 0, dur = 30;
    uint32_t lcd_tick = 0;
    char buf[17];
    printf("\r\n===== Exp5: Traffic Light =====\r\n");
    TrafficLight_Init();
    TrafficLight_Set(2, 0); Buzzer_Set(0);
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    while (1) {
        if (stop_requested) { TrafficLight_Set(0,0); Buzzer_Set(0); LCD12864_Clear(); LCD12864_Backlight(0); return; }
        ticks++;
        switch (s) {
            case TL_NS_G_WE_R: if (ticks>=dur){s=TL_NS_Y_WE_R;dur=10;ticks=0;TrafficLight_Set(1,0);printf("NS:Yel WE:Red\r\n");} break;
            case TL_NS_Y_WE_R: if (ticks>=dur){s=TL_NS_R_WE_G;dur=30;ticks=0;TrafficLight_Set(0,2);printf("NS:Red WE:Grn\r\n");} break;
            case TL_NS_R_WE_G: if (ticks>=dur){s=TL_NS_R_WE_Y;dur=10;ticks=0;TrafficLight_Set(0,1);printf("NS:Red WE:Yel\r\n");} break;
            case TL_NS_R_WE_Y: if (ticks>=dur){s=TL_NS_G_WE_R;dur=30;ticks=0;TrafficLight_Set(2,0);printf("NS:Grn WE:Red\r\n");Buzzer_Set(1);delay_ms(100);Buzzer_Set(0);} break;
        }
        if (++lcd_tick >= 5) {  /* every 500ms */
            lcd_tick = 0;
            LCD12864_SetPos(0,0);
            switch(s){
                case TL_NS_G_WE_R: LCD12864_WriteString("NS:Grn   "); break;
                case TL_NS_Y_WE_R: LCD12864_WriteString("NS:Yel   "); break;
                case TL_NS_R_WE_G: LCD12864_WriteString("WE:Grn   "); break;
                case TL_NS_R_WE_Y: LCD12864_WriteString("WE:Yel   "); break;
            }
            LCD12864_SetPos(1,0);
            sprintf(buf,"T=%lu/%-3lu",ticks,dur); LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0); LCD12864_WriteString("Traffic ");
            LCD12864_SetPos(3,0); LCD12864_WriteString("Normal  ");
        }
        delay_ms(100);
    }
}

/* ============================================================
 *  Exp2: Key Polling  (IKEY0-3 → LED5-8)
 * ============================================================ */
void Exp2_KeyPolling(void)
{
    uint8_t ks[4]={0}, ls[4]={0};
    uint8_t i;
    uint32_t lcd_tick = 0;
    char buf[17];
    LED_Init(); IKEY_Init();
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp2: Key Polling =====\r\n");
    while (1) {
        if (stop_requested) { LED_AllOff(); LCD12864_Clear(); LCD12864_Backlight(0); return; }
        for (i=0;i<4;i++) ks[i]=IKEY_Read(i);
        for (i=0;i<4;i++) {
            if (ks[i]!=ls[i]) { ls[i]=ks[i]; LED_Set(i+4,ks[i]); printf("IKEY%d %s\r\n",i,ks[i]?"PRESSED":"released"); }
        }
        if (++lcd_tick >= 10) {  /* every 500ms (10x50ms) */
            lcd_tick = 0;
            LCD12864_SetPos(0,0); LCD12864_WriteString("Key Poll");
            LCD12864_SetPos(1,0);
            sprintf(buf,"0=%d 1=%d  ",ks[0],ks[1]); LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0);
            sprintf(buf,"2=%d 3=%d  ",ks[2],ks[3]); LCD12864_WriteString(buf);
            LCD12864_SetPos(3,0);
            sprintf(buf,"LED:%d%d%d%d  ",ks[3],ks[2],ks[1],ks[0]);
            LCD12864_WriteString(buf);
        }
        delay_ms(50);
    }
}

/* ============================================================
 *  Exp8: Independent Keys EXTI  (IKEY0-3 EXTI → toggle LED5-8)
 * ============================================================ */
void Exp8_IndependentKeys(void)
{
    uint8_t led_t[4]={0};
    uint8_t i;
    uint8_t ks[4], ls[4]={0};
    uint32_t lcd_tick = 0;
    char buf[17];
    LED_Init(); IKEY_EXTI_Init();  /* EXTI only for IKEY2(PE2)+IKEY3(PE3) */
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp8: Independent Keys (EXTI+Poll) =====\r\n");
    printf("IKEY0/1=polling  IKEY2/3=EXTI\r\n");
    while (1) {
        if (stop_requested) { LED_AllOff(); IKEY_EXTI_DeInit(); LCD12864_Clear(); LCD12864_Backlight(0); return; }

        /* IKEY2/3: EXTI-driven (check ISR flags) */
        for (i = 2; i < 4; i++) {
            if (ikey_pressed[i]) {
                ikey_pressed[i] = 0;
                delay_ms(10);
                if (IKEY_Read(i)) {
                    led_t[i] = !led_t[i];
                    LED_Set(i + 4, led_t[i]);
                    printf("IKEY%d -> LED%d %s\r\n", i, i + 5, led_t[i] ? "ON" : "OFF");
                }
            }
        }

        /* IKEY0/1: polling (no EXTI — pins share EXTI lines with IKEY2/3) */
        for (i = 0; i < 2; i++) ks[i] = IKEY_Read(i);
        for (i = 0; i < 2; i++) {
            if (!ls[i] && ks[i]) {  /* rising edge (released → pressed) */
                delay_ms(10);
                if (IKEY_Read(i)) {
                    led_t[i] = !led_t[i];
                    LED_Set(i + 4, led_t[i]);
                    printf("IKEY%d -> LED%d %s\r\n", i, i + 5, led_t[i] ? "ON" : "OFF");
                }
            }
            ls[i] = ks[i];
        }
        if (++lcd_tick >= 25) {  /* every 500ms (25x20ms) */
            lcd_tick = 0;
            LCD12864_SetPos(0,0); LCD12864_WriteString("Key EXTI");
            LCD12864_SetPos(1,0);
            sprintf(buf,"0=%dP 1=%dP",ks[0],ks[1]); LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0);
            sprintf(buf,"2=%dE 3=%dE",IKEY_Read(2),IKEY_Read(3)); LCD12864_WriteString(buf);
            LCD12864_SetPos(3,0);
            sprintf(buf,"L:%d%d%d%d  ",led_t[3],led_t[2],led_t[1],led_t[0]); LCD12864_WriteString(buf);
        }
        delay_ms(20);
    }
}

/* ============================================================
 *  Exp9: DIP Switch  (SW0-7 → LED1-4 + serial)
 * ============================================================ */
void Exp9_DIPSwitch(void)
{
    uint8_t sw, last=0xFF;
    uint8_t i;
    uint32_t lcd_tick = 0;
    char buf[17];
    LED_Init(); SW_Init();
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp9: DIP Switch =====\r\n");
    while (1) {
        if (stop_requested) { LED_AllOff(); LCD12864_Clear(); LCD12864_Backlight(0); return; }
        sw = SW_ReadAll();
        if (sw != last) { last=sw; for(i=0;i<4;i++) LED_Set(i,(sw>>i)&1); printf("DIP:%d%d%d%d%d%d%d%d 0x%02X\r\n",(sw>>7)&1,(sw>>6)&1,(sw>>5)&1,(sw>>4)&1,(sw>>3)&1,(sw>>2)&1,(sw>>1)&1,sw&1,sw); }
        if (++lcd_tick >= 5) {  /* every 1s (5x200ms) */
            lcd_tick = 0;
            LCD12864_SetPos(0,0); LCD12864_WriteString("DIP SW  ");
            LCD12864_SetPos(1,0);
            for(i=0;i<8;i++) buf[i]=(sw>>(7-i))&1?'1':'0'; buf[8]=0;
            LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0);
            sprintf(buf,"HEX=0x%02X",sw); LCD12864_WriteString(buf);
            LCD12864_SetPos(3,0); LCD12864_WriteString("SW0-7   ");
        }
        delay_ms(200);
    }
}

/* ============================================================
 *  Exp4: USART1 Serial Echo
 * ============================================================ */
void Exp4_USART1_Echo(void)
{
    uint8_t ch;
    uint32_t lcd_tick = 0;
    uint16_t rx_cnt = 0;
    char buf[17];
    uint8_t last_ch = 0;
    USART_RX_Init(); USART_RX_Flush();
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp4: USART1 Serial Echo =====\r\n");
    while (1) {
        if (stop_requested) { USART_RX_DeInit(); LCD12864_Clear(); LCD12864_Backlight(0); return; }
        while (USART_RX_Available()) { ch=USART_RX_Read(); last_ch=ch; rx_cnt++; printf("Echo:'%c' 0x%02X\r\n",(ch>=32&&ch<127)?ch:'.',ch); }
        if (++lcd_tick >= 50) {  /* every 1s (50x20ms) */
            lcd_tick = 0;
            LCD12864_SetPos(0,0); LCD12864_WriteString("USART1  ");
            LCD12864_SetPos(1,0);
            sprintf(buf,"RX=%-5d",rx_cnt); LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0);
            sprintf(buf,"Last:%c ",(last_ch>=32&&last_ch<127)?last_ch:'.'); LCD12864_WriteString(buf);
            LCD12864_SetPos(3,0); LCD12864_WriteString("115200  ");
        }
        delay_ms(20);
    }
}

/* ============================================================
 *  Exp15: RS232 (USART3)
 * ============================================================ */
void Exp15_RS232(void)
{
    uint32_t tick=0;
    uint8_t ch;
    uint16_t rx_cnt = 0;
    uint8_t last_ch = 0;
    uint32_t lcd_tick = 0;
    char buf[17];

    RS232_Init(115200);
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp15: RS232 (USART3 remap PD8/PD9) =====\r\n");
    printf("PD8=TX PD9=RX (remapped from PB10/PB11)\r\n");
    printf("Short PD8--PD9 for loopback test\r\n");
    RS232_SendString("RS232 Test Start\r\n");

    while (1) {
        if (stop_requested) {
            USART_Cmd(RS232_USART,DISABLE);
            LCD12864_Clear(); LCD12864_Backlight(0);
            return;
        }
        if (++tick % 20 == 0) {
            RS232_SendString("RS232 ping\r\n");
            printf("[RS232] Sent:ping\r\n");
        }
        while (RS232_RxAvailable()) {
            ch=RS232_ReadByte(); last_ch=ch; rx_cnt++;
            printf("[RS232] RX:'%c' 0x%02X\r\n",(ch>=32&&ch<127)?ch:'.',ch);
        }
        if (++lcd_tick >= 5) {  /* every 500ms */
            lcd_tick = 0;
            LCD12864_SetPos(0,0); LCD12864_WriteString("RS232 USART3     ");
            LCD12864_SetPos(1,0);
            sprintf(buf,"TX=PD8 RX=PD9    "); LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0);
            sprintf(buf,"RX=%-5d          ",rx_cnt); LCD12864_WriteString(buf);
            LCD12864_SetPos(3,0);
            sprintf(buf,"Last:%c            ",(last_ch>=32&&last_ch<127)?last_ch:'.'); LCD12864_WriteString(buf);
        }
        delay_ms(100);
    }
}

void Exp16_RS485(void)
{
    uint32_t tick=0;
    uint8_t ch;
    uint16_t rx_cnt = 0;
    uint32_t lcd_tick = 0;
    char buf[17];
    uint8_t last_ch = 0;
    RS485_Init(115200);
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp16: RS485 (USART4) =====\r\n");
    RS485_SendString("RS485 Test Start\r\n"); printf("[RS485] Sent. Loopback: short PC10(P40)--PC11(P41)\r\n");
    while (1) {
        if (stop_requested) { USART_Cmd(RS485_USART,DISABLE); LCD12864_Clear(); LCD12864_Backlight(0); return; }
        if (++tick % 20 == 0) { RS485_SendString("RS485 ping\r\n"); printf("[RS485] Sent:ping\r\n"); }
        while (RS485_RxAvailable()) { ch=RS485_ReadByte(); last_ch=ch; rx_cnt++; printf("[RS485] RX:'%c' 0x%02X\r\n",(ch>=32&&ch<127)?ch:'.',ch); }
        if (++lcd_tick >= 5) {  /* every 500ms */
            lcd_tick = 0;
            LCD12864_SetPos(0,0); LCD12864_WriteString("RS485   ");
            LCD12864_SetPos(1,0);
            sprintf(buf,"RX=%-5d",rx_cnt); LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0);
            sprintf(buf,"Last:%c ",(last_ch>=32&&last_ch<127)?last_ch:'.'); LCD12864_WriteString(buf);
            LCD12864_SetPos(3,0); LCD12864_WriteString("Short P40-41");
        }
	        delay_ms(100);
    }
}

/* ============================================================
 *  Exp3: Light Sensor ADC  (PA1 -> LED bar + serial)
 * ============================================================ */
void Exp3_LightSensor(void)
{
    uint8_t pct, last_pct = 0xFF, i, n;
    uint32_t lcd_tick = 0;
    char buf[17];
    LED_Init(); ADC1_Init();
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp3: Light Sensor (ADC1 PA1) =====\r\n");
    printf("LED1-8 = brightness bar. Cover photoresistor to see change.\r\n");
    while (1) {
        if (stop_requested) { LED_AllOff(); LCD12864_Clear(); LCD12864_Backlight(0); return; }
        pct = Light_Percent();
        if (pct != last_pct) {
            last_pct = pct;
            printf("Light: %d%% (ADC=%d)\r\n", pct, ADC1_ReadAvg(ADC_LIGHT_CH,4));
            n = pct / 13;
            for (i=0;i<8;i++) LED_Set(i, (i<=n)?1:0);
        }
        if (++lcd_tick >= 5) {  /* every 1s (5x200ms) */
            lcd_tick = 0;
            LCD12864_SetPos(0,0); LCD12864_WriteString("Light   ");
            LCD12864_SetPos(1,0);
            sprintf(buf,"L=%d%%    ",pct); LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0);
            n = pct / 13;
            for(i=0;i<8;i++) buf[i]=(i<=n)?'=':' '; buf[8]=0;
            LCD12864_WriteString(buf);
            LCD12864_SetPos(3,0);
            sprintf(buf,"ADC=%-4d",ADC1_ReadAvg(ADC_LIGHT_CH,4)); LCD12864_WriteString(buf);
        }
        delay_ms(200);
    }
}

/* ============================================================
 *  Exp11: DHT11 Temperature & Humidity  (PA5 One-Wire)
 * ============================================================ */
void Exp11_DHT11(void)
{
    uint8_t tick;
    DHT11_Data d;
    uint8_t last_hum=0xFF, last_temp=0xFF;
    uint8_t status=0;
    char buf[17];
    DHT11_Init();
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp11: DHT11 Temp & Humidity =====\r\n");
    printf("PA5 One-Wire, read interval ~2s\r\n");
    while (1) {
        for (tick = 0; tick < 40; tick++) {
            if (stop_requested) { LCD12864_Clear(); LCD12864_Backlight(0); return; }
            delay_ms(50);
        }
        d = DHT11_Read();
        status = d.status;
        if (d.status == DHT11_OK) {
            printf("Humidity: %d%%  Temperature: %d C\r\n", d.humidity, d.temperature);
            last_hum = d.humidity; last_temp = d.temperature;
        } else if (d.status == DHT11_CHKSUM)
            printf("DHT11 checksum error\r\n");
        else
            printf("DHT11 timeout (check wiring / pull-up)\r\n");

        LCD12864_SetPos(0,0); LCD12864_WriteString("DHT11   ");
        LCD12864_SetPos(1,0);
        if(status==0) sprintf(buf,"H=%-3d%%  ",last_hum); else sprintf(buf,"H=ERR    ");
        LCD12864_WriteString(buf);
        LCD12864_SetPos(2,0);
        if(status==0) sprintf(buf,"T=%-3dC  ",last_temp); else sprintf(buf,"T=ERR    ");
        LCD12864_WriteString(buf);
        LCD12864_SetPos(3,0);
        LCD12864_WriteString(status==0?"OK      ":(status==2?"CHKSUM  ":"TIMEOUT "));
    }
}

/* ============================================================
 *  Exp12: Thermistor ADC  (PC1 → serial)
 * ============================================================ */
void Exp12_Thermistor(void)
{
    uint16_t val, last_val = 0xFFFF;
    uint32_t lcd_tick = 0;
    char buf[17];
    uint32_t mv;
    ADC1_Init();
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp12: Thermistor ADC (PC1) =====\r\n");
    printf("PC1 = ADC123_IN11. NTC voltage divider.\r\n");
    printf("Raw ADC value (0=0V, 4095=3.3V).\r\n");
    while (1) {
        if (stop_requested) { LCD12864_Clear(); LCD12864_Backlight(0); return; }
        val = ADC1_ReadAvg(ADC_TEMP_CH, 8);
        if (val != last_val) {
            last_val = val;
            printf("Thermistor: %d (%.3fV)\r\n", val, val * 3.3f / 4095.0f);
        }
        if (++lcd_tick >= 5) {  /* every 1s */
            lcd_tick = 0;
            mv = (uint32_t)val * 3300 / 4095;
            LCD12864_SetPos(0,0); LCD12864_WriteString("Thermist");
            LCD12864_SetPos(1,0);
            sprintf(buf,"ADC=%-4d",val); LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0);
            sprintf(buf,"V=%lu.%-3luV",mv/1000,mv%1000); LCD12864_WriteString(buf);
            LCD12864_SetPos(3,0); LCD12864_WriteString("PC1 IN11");
        }
        delay_ms(200);
    }
}

/* ============================================================
 *  Exp13: DS18B20 Temperature  (PE9 One-Wire)
 * ============================================================ */
void Exp13_DS18B20(void)
{
    uint8_t tick;
    int16_t last_temp = -999;
    uint8_t status = DS18B20_OK;
    char buf[17];
    DS18B20_Init();
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp13: DS18B20 Temperature =====\r\n");
    printf("PE9 One-Wire. 2s cycle.\r\n");
    while (1) {
        for (tick = 0; tick < 40; tick++) {
            if (stop_requested) { LCD12864_Clear(); LCD12864_Backlight(0); return; }
            delay_ms(50);
        }
        DS18B20_StartConvert();
        for (tick = 0; tick < 16; tick++) {
            if (stop_requested) { LCD12864_Clear(); LCD12864_Backlight(0); return; }
            delay_ms(50);
        }
        DS18B20_Data d = DS18B20_ReadTemp();
        status = d.status;
        if (d.status == DS18B20_OK) {
            int t = d.temp_x10;
            printf("Temp: %c%d.%d C\r\n", (t<0)?'-':' ', (t<0?-t:t)/10, (t<0?-t:t)%10);
            last_temp = d.temp_x10;
        } else {
            printf("DS18B20 not found (check wiring / pull-up)\r\n");
        }

        LCD12864_SetPos(0,0); LCD12864_WriteString("DS18B20 ");
        LCD12864_SetPos(1,0);
        if(status==DS18B20_OK){
            int t=last_temp; sprintf(buf,"%c%d.%-2dC",(t<0)?'-':' ',(t<0?-t:t)/10,(t<0?-t:t)%10); LCD12864_WriteString(buf);
        } else LCD12864_WriteString("No Dev  ");
        LCD12864_SetPos(2,0); LCD12864_WriteString("PE9 1W  ");
        LCD12864_SetPos(3,0);
        LCD12864_WriteString(status==DS18B20_OK?"OK      ":"TIMEOUT ");
    }
}

/* ============================================================
 *  Exp17: Infrared Receiver  (PC0, HS0038, NEC decode)
 * ============================================================ */
void Exp17_Infrared(void)
{
    uint8_t last_addr=0, last_cmd=0;
    uint32_t repeat_cnt=0;
    uint32_t lcd_tick = 0;
    char buf[17];
    IR_Init();
    LED_Init();
    LED_AllOff();
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp17: Infrared Receiver =====\r\n");
    printf("PC0 -> HS0038. Point remote and press buttons.\r\n");
    printf("Long-press WKUP (>500ms) to switch\r\n\r\n");
    while (1) {
        if (stop_requested) { IR_EXTI_Disable(); LCD12864_Clear(); LCD12864_Backlight(0); return; }

        if (ir_received) {
            ir_received = 0;
            IR_EXTI_Disable();
            IR_Data ir = IR_Decode();
            IR_EXTI_Enable();
            if (ir.valid) {
	                if (ir.is_repeat) { printf("IR: Repeat\r\n"); repeat_cnt++; }
	                else {
	                    printf("IR: Addr=0x%02X  Cmd=0x%02X\r\n", ir.addr, ir.cmd);
	                    last_addr=ir.addr; last_cmd=ir.cmd; repeat_cnt=0;
	                    /* LED control: keys 0-3 control LED1-3 via IR remote */
	                    if (ir.addr == 0x00) {
	                        LED_AllOff();
	                        if (ir.cmd == 0x0C)      { LED_Set(0,1); }                  /* key 1 -> LED1 */
	                        else if (ir.cmd == 0x18) { LED_Set(0,1); LED_Set(1,1); }    /* key 2 -> LED1+2 */
	                        else if (ir.cmd == 0x5E) { LED_Set(0,1); LED_Set(1,1); LED_Set(2,1); } /* key 3 -> LED1-3 */
	                        /* key 0 (0x16) -> all off (already done by LED_AllOff) */
	                    }
	                }
	            }
        }

        if (++lcd_tick >= 500) {  /* ~500ms (500x1ms). Throttle LCD to reduce IR interference */
            lcd_tick = 0;
            LCD12864_SetPos(0,0); LCD12864_WriteString("IR Recv ");
            LCD12864_SetPos(1,0);
            sprintf(buf,"A=0x%02X   ",last_addr); LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0);
            sprintf(buf,"C=0x%02X   ",last_cmd); LCD12864_WriteString(buf);
            LCD12864_SetPos(3,0);
            sprintf(buf,"LED:%d%d%d       ",(last_cmd==0x0C||last_cmd==0x18||last_cmd==0x5E)?1:0,(last_cmd==0x18||last_cmd==0x5E)?1:0,(last_cmd==0x5E)?1:0); LCD12864_WriteString(buf);
        }

        delay_ms(1);  /* 1ms fast polling needed for IR NEC decode */
    }
}

/* ============================================================
 *  Exp18: Photoelectric Encoder Quadrature A/B/Z  (PB6=A PB7=B PD15=Z)
 * ============================================================ */
void Exp18_Encoder(void)
{
    int32_t count, last_count = -999;
    uint32_t lcd_tick = 0;
    uint8_t dir_state;
    char buf[17];
    ENC_Init();
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp18: Encoder (Quad A/B/Z) =====\r\n");
    printf("PB6=A(OUT) PB7=B(DIR) PD15=Z\r\n");
    printf("Rotate encoder, Z to reset.\r\n");
    while (1) {
        if (stop_requested) { LCD12864_Clear(); LCD12864_Backlight(0); return; }
        if (ENC_Z_IsPressed()) { ENC_Reset(); printf("Enc: Z(reset)\r\n"); delay_ms(200); }
        count = ENC_GetCount();
        dir_state = (GPIO_ReadInputDataBit(ENC_B_PORT, ENC_B_PIN) != Bit_RESET) ? 1 : 0;
        if (count != last_count) {
            last_count = count;
            printf("Encoder: %ld  B(PB7)=%d\r\n", (long)count, dir_state);
        }
        if (++lcd_tick >= 10) {  /* every 500ms (10x50ms) */
            lcd_tick = 0;
            LCD12864_SetPos(0,0); LCD12864_WriteString("Encoder ");
            LCD12864_SetPos(1,0);
            sprintf(buf,"Cnt=%-4ld",(long)count); LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0);
            sprintf(buf,"B=%d     ",dir_state); LCD12864_WriteString(buf);
            LCD12864_SetPos(3,0); LCD12864_WriteString("Z=Reset ");
        }
        delay_ms(50);
    }
}

/* ============================================================
 *  Exp19: DC Motor PWM  (PA6=PWMA=speed, PA7=direction)
 * ============================================================ */
void Exp19_DCMotorPWM(void)
{
    uint32_t lcd_tick = 0;


    Motor_Init();
    Motor_SetDir(0);    /* forward only, no reverse */
    Motor_SetSpeed(10); /* low constant speed */
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp19: DC Motor PWM =====\r\n");
    printf("PA6=PWMA PA7=PWMB  (TIM3_CH1/CH2)\r\n");
    printf("Forward constant 10%% speed. No auto-reverse.\r\n");

    while (1) {
        if (stop_requested) { Motor_Stop(); LCD12864_Clear(); LCD12864_Backlight(0); return; }

        if (++lcd_tick >= 5) {  /* every 500ms */
            lcd_tick = 0;
            LCD12864_SetPos(0,0); LCD12864_WriteString("DC Motor");
            LCD12864_SetPos(1,0); LCD12864_WriteString("FWD     ");
            LCD12864_SetPos(2,0); LCD12864_WriteString("Spd=10% ");
            LCD12864_SetPos(3,0); LCD12864_WriteString("TIM3 CH1");
        }
	        delay_ms(100);
    }
}

/* ============================================================
 *  Exp20: Servo  (PB9=TIM4_CH4, 50Hz)
 * ============================================================ */
void Exp20_Servo(void)
{
    uint32_t tick = 0;
    uint8_t ang_idx = 0;
    uint8_t cur_ang = 90;
    const uint8_t angles[3] = {0, 90, 180};
    char buf[17];
    TIM4_Servo_Init();
    TIM4_Servo_SetAngle(90);
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp20: Servo =====\r\n");
    printf("PB8=TIM4_CH3 50Hz PWM (verified OK)\r\n");
    printf("Angle auto-cycle: 0->90->180 every 3s.\r\n");
    while (1) {
        if (stop_requested) { LCD12864_Clear(); LCD12864_Backlight(0); return; }
        if (++tick % 30 == 0) {
            cur_ang = angles[ang_idx];
            ang_idx = (ang_idx + 1) % 3;
            TIM4_Servo_SetAngle(cur_ang);
            printf("Angle: %d deg\r\n", cur_ang);
        }
        if (tick % 5 == 0) {  /* every 500ms */
            LCD12864_SetPos(0,0); LCD12864_WriteString("Servo   ");
            LCD12864_SetPos(1,0);
            sprintf(buf,"Ang=%-3d ",cur_ang); LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0); LCD12864_WriteString("PB8     ");
            LCD12864_SetPos(3,0); LCD12864_WriteString("TIM4 CH3");
        }
	        delay_ms(100);
    }
}

/* ============================================================
 *  Exp6: LCD 12864 Serial Display
 * ============================================================ */
void Exp6_LCD12864(void)
{
    uint32_t tick = 0;
    uint8_t page = 0;
    LCD12864_Init();
    LCD12864_Clear();
    printf("\r\n===== Exp6: LCD 12864 =====\r\n");
    while (1) {
        if (stop_requested) {
            LCD12864_Clear();
            LCD12864_Backlight(0);
            return;
        }
        if (++tick % 30 == 0) {
            page = (page + 1) % 4;
            LCD12864_Clear();
            switch (page) {
                case 0:
                    LCD12864_SetPos(0, 0);
                    LCD12864_WriteString("Student:");
                    LCD12864_SetPos(1, 0);
                    LCD12864_WriteString("XuQian  ");
                    LCD12864_SetPos(2, 0);
                    LCD12864_WriteString("ID:     ");
                    LCD12864_SetPos(3, 0);
                    LCD12864_WriteString("666666668888   ");
                    printf("[LCD] Pg0:Student\r\n");
                    break;
                case 1:
                    LCD12864_SetPos(0, 0);
                    LCD12864_WriteString("V5+MainBd");
                    LCD12864_SetPos(1, 0);
                    LCD12864_WriteString("STM32F1 ");
                    LCD12864_SetPos(2, 0);
                    LCD12864_WriteString("VET6    ");
                    LCD12864_SetPos(3, 0);
                    LCD12864_WriteString("FreeRTOS");
                    printf("[LCD] Pg1:ChipInfo\r\n");
                    break;
                case 2:
                    LCD12864_SetPos(0, 0);
                    LCD12864_WriteString("V5 Board");
                    LCD12864_SetPos(1, 0);
                    LCD12864_WriteString("72MHz   ");
                    LCD12864_SetPos(2, 0);
                    LCD12864_WriteString("LCD 1286");
                    LCD12864_SetPos(3, 0);
                    LCD12864_WriteString("4 OK!   ");
                    printf("[LCD] Pg2:System \r\n");
                    break;
                case 3:
                    LCD12864_SetPos(0, 0);
                    LCD12864_WriteString("21-Exper");
                    LCD12864_SetPos(1, 0);
                    LCD12864_WriteString("iments  ");
                    LCD12864_SetPos(2, 0);
                    LCD12864_WriteString("All Pass");
                    LCD12864_SetPos(3, 0);
                    LCD12864_WriteString("No Error");
                    printf("[LCD] Pg3:Status \r\n");
                    break;
            }
        }
        delay_ms(100);
    }
}

void Exp7_CH451(void)
{
	    uint32_t tick = 0;
	    int32_t counter = 0;
	    uint8_t key = 0xFF;
    uint8_t last_key = 0xFF;
    uint32_t lcd_tick = 0;
    char buf[17];

    CH451_Init();
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp7: CH451 7-Seg + Keyboard =====\r\n");
    printf("PD8=DCLK PD9=DIN PD10=DOUT PD11=LOAD\r\n");
    printf("8-digit 7-seg display. Press matrix keys.\r\n");
    printf("Counter auto-increments. Keys shown on serial.\r\n");

    CH451_DisplayNum(counter);

    while (1) {
        if (stop_requested) {
            CH451_Clear();
            LCD12864_Clear(); LCD12864_Backlight(0);
            return;
        }

        /* Poll keyboard */
        key = CH451_ReadKey();
        if (key != 0xFF) {
            printf("[CH451] Key: %d (0x%02X)\r\n", key, key);
            CH451_DisplayNum(key);
            delay_ms(500);
            CH451_DisplayNum(counter);
            last_key = key;
        }

        if (++tick % 5 == 0) {
            counter = (counter + 1) % 10000;
            CH451_DisplayNum(counter);
        }
        if (++lcd_tick >= 5) {  /* every 500ms */
            lcd_tick = 0;
            LCD12864_SetPos(0,0); LCD12864_WriteString("CH451   ");
            LCD12864_SetPos(1,0);
            sprintf(buf,"Key=%-3d",last_key); LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0);
            sprintf(buf,"Cnt=%-4d",(int)counter); LCD12864_WriteString(buf);
            LCD12864_SetPos(3,0); LCD12864_WriteString("PD8-11  ");
        }
	        delay_ms(100);
    }
}

/* ============================================================
 *  Exp10: Relay  (PD9=RELAY1, PD10=RELAY2)
 * ============================================================ */
void Exp10_Relay(void)
{
    uint32_t tick = 0;
    uint8_t state = 0;
    char buf[17];

    LED_Init();
    Relay_Init();
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp10: Relay =====\r\n");
    printf("PD9=RELAY1 PD10=RELAY2\r\n");
    printf("LED1=RELAY1 LED2=RELAY2 (core board LEDs)\r\n");
    printf("Auto-cycle: OFF->R1->R2->BOTH every 2s\r\n");

    while (1) {
        if (stop_requested) {
            Relay_Set(1, 0); Relay_Set(2, 0);
            LED_AllOff();
            LCD12864_Clear(); LCD12864_Backlight(0);
            return;
        }

        if (++tick % 20 == 0) {
            state = (state + 1) % 4;
            switch (state) {
                case 0:
                    Relay_Set(1, 0); Relay_Set(2, 0);
                    LED_Set(0, 0); LED_Set(1, 0);
                    printf("Relay: Both OFF\r\n");
                    break;
                case 1:
                    Relay_Set(1, 1); Relay_Set(2, 0);
                    LED_Set(0, 1); LED_Set(1, 0);
                    printf("Relay: R1 ON, R2 OFF\r\n");
                    break;
                case 2:
                    Relay_Set(1, 0); Relay_Set(2, 1);
                    LED_Set(0, 0); LED_Set(1, 1);
                    printf("Relay: R1 OFF, R2 ON\r\n");
                    break;
                case 3:
                    Relay_Set(1, 1); Relay_Set(2, 1);
                    LED_Set(0, 1); LED_Set(1, 1);
                    printf("Relay: Both ON\r\n");
                    break;
            }
        }
        if (tick % 5 == 0) {  /* every 500ms */
            LCD12864_SetPos(0,0); LCD12864_WriteString("Relay   ");
            LCD12864_SetPos(1,0);
            if(state==0) LCD12864_WriteString("OFF/OFF ");
            else if(state==1) LCD12864_WriteString("ON /OFF ");
            else if(state==2) LCD12864_WriteString("OFF/ON  ");
            else LCD12864_WriteString("ON /ON  ");
            LCD12864_SetPos(2,0); LCD12864_WriteString("PD9/10  ");
            LCD12864_SetPos(3,0);
            sprintf(buf,"St=%-2d/4",state); LCD12864_WriteString(buf);
        }
	        delay_ms(100);
    }
}

/* ============================================================
 *  Exp14: TLC5615 DA Output  (PB12=CS PB13=SCLK PB15=DIN)
 * ============================================================ */
void Exp14_DA(void)
{
    uint32_t tick = 0;
    uint16_t dac_code = 0;
    uint8_t pct = 0;
    uint16_t adc_val = 0;
    uint16_t adc_mv = 0;
    char buf[17];
    uint8_t i, on_steps;

    LED_Init();
    LED_AllOff();
    TLC5615_Init();
    TLC5615_SetValue(0);
    ADC1_Init();
    LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp14: DA->ADC->LED1 Closed Loop =====\r\n");
    printf("DA-OUT->P120(PC1 ADC)->LCD+PWM LED1\r\n");
    printf("1 fly wire: DA-OUT--P120\r\n");

    while (1) {
        if (stop_requested) {
            TLC5615_SetValue(0);
            LED_AllOff();
            LCD12864_Clear(); LCD12864_Backlight(0);
            return;
        }

        /* Software PWM on LED1(PC2) follows pct */
        on_steps = (uint8_t)((uint32_t)pct * 10 / 100);
        for (i = 0; i < 10; i++) {
            if (i < on_steps) LED_Set(0, 1); else LED_Set(0, 0);
            delay_us(100);
        }

        /* Read DA-OUT via PC1(P120) ADC */
        adc_val = ADC1_ReadAvg(ADC_Channel_11, 4);
        adc_mv  = (uint16_t)((uint32_t)adc_val * 3300 / 4095);

        /* Ramp DA output every 4s */
        if (++tick >= 40) {
            tick = 0;
            pct += 10;
            if (pct > 100) pct = 0;
            dac_code = (uint16_t)(((uint32_t)pct * 1023) / 100);
            TLC5615_SetValue(dac_code);
            printf("DA code=%d (%d%%), ADC=%dmV, LED1=%d%%\r\n", dac_code, pct, adc_mv, pct);
        }

        /* LCD update every 500ms */
        if (tick % 5 == 0) {
            LCD12864_SetPos(0,0); LCD12864_WriteString("DA->ADC->LED1    ");
            LCD12864_SetPos(1,0);
            sprintf(buf,"ADC=%-4d %-4dmV   ",adc_val,adc_mv); LCD12864_WriteString(buf);
            LCD12864_SetPos(2,0);
            {
                uint8_t n = (uint8_t)((uint32_t)pct * 8 / 100);
                uint8_t j;
                for(j=0;j<8;j++) buf[j]=(j<n)?'=':' ';
                buf[8]=0;
                LCD12864_WriteString(buf);
            }
            LCD12864_SetPos(3,0);
            sprintf(buf,"Bright=%-3d%%      ",pct); LCD12864_WriteString(buf);
        }
    }
}


void Exp18_StepperMotor(void)
{
    uint32_t tick = 0;
    int32_t count, last_count = -999;
    uint8_t dir = 0;
    uint32_t lcd_update = 0;
    char buf[17];
    uint16_t step_progress = 0;

    Stepper_Init();
    ENC_Init();
        LCD12864_Init(); LCD12864_Clear(); LCD12864_Backlight(1);
    printf("\r\n===== Exp18: Stepper Motor =====\r\n");
    printf("PE12=A PE13=B PE10=C PE11=D\r\n");
    printf("Encoder: PB6=OUT PB7=DIR PD15=Z\r\n");
    printf("Auto: CW 256 steps -> pause -> CCW 256 steps -> repeat\r\n");

    while (1) {
        if (stop_requested) { Stepper_Stop(); LCD12864_Clear(); LCD12864_Backlight(0); return; }

        if (ENC_Z_IsPressed()) { ENC_Reset(); count = 0; last_count = 0; }

        if (++tick % 30 == 0) {
            if (dir == 0) {
                printf("Stepper: CW 256 steps...\r\n");
                Stepper_StepCW(256);
            } else {
                printf("Stepper: CCW 256 steps...\r\n");
                Stepper_StepCCW(256);
            }
            dir = !dir;
            count = ENC_GetCount();
            printf("Encoder: %ld\r\n", (long)count);
            step_progress = 0;
        }
        step_progress++;

        count = ENC_GetCount();
        if (count != last_count) { last_count = count; printf("Enc: %ld\r\n", (long)count); }

        if (++lcd_update >= 5) {  /* every 500ms */
            lcd_update = 0;
            LCD12864_SetPos(0,0); LCD12864_WriteString("Stepper ");
            LCD12864_SetPos(1,0);
            LCD12864_WriteString(dir?"CCW    ":"CW     ");
            LCD12864_SetPos(2,0);
            sprintf(buf,"Sp=%lu/30",(unsigned long)step_progress); LCD12864_WriteString(buf);
            LCD12864_SetPos(3,0);
            sprintf(buf,"Enc=%-4ld",(long)count); LCD12864_WriteString(buf);
        }

	        delay_ms(100);
    }
}

/* ============================================================
 *  main  — hardware init → create RTOS objects → start scheduler
 * ============================================================ */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    delay_us_init();

    LED_Init();
    WKUP_Init();
    usart_init(115200);

    printf("\r\n=================================\r\n");printf("  V5 CoreBoard + MainBoard\r\n");
	    printf("  21-Function + FreeRTOS V9.0.0\r\n");
    printf("=================================\r\n");

    /* Hold motor pins LOW at boot */
    {
        GPIO_InitTypeDef m;
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
        m.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
        m.GPIO_Speed = GPIO_Speed_50MHz;
        m.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_Init(GPIOA, &m);
        GPIO_ResetBits(GPIOA, GPIO_Pin_6 | GPIO_Pin_7);
    }

    printf("2 Tasks: Control(pri4) + Experiment(pri3)\r\n");
    printf("Long-press WKUP (>500ms) to cycle modes.\r\n\r\n");

    freertos_init();

    vTaskStartScheduler();

    /* Should never reach here */
    while (1);
}
