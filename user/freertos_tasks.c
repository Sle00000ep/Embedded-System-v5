/**
 * freertos_tasks.c — FreeRTOS 2-task integration (Exp21)
 *
 * ControlTask  (prio 4, 256W): polls WKUP PA0 every 50ms,
 *   on 500ms long-press sets stop_requested and sends next mode to queue.
 *   Sets scheduler_started=1 on first entry (used by delay_ms).
 * ExperimentTask (prio 3, 512W): blocks on queue, runs one experiment at a time.
 */

#include "freertos_tasks.h"
#include "stm32f10x.h"
#include <stdio.h>

/* ---- Global RTOS objects ---- */
QueueHandle_t     xModeQueue           = NULL;
volatile uint8_t  stop_requested       = 0;
volatile uint8_t  scheduler_started    = 0;
TaskHandle_t      xControlTaskHandle   = NULL;
TaskHandle_t      xExperimentTaskHandle = NULL;

/* ---- WKUP helpers (defined in main.c) ---- */
extern uint8_t WKUP_IsPressed(void);

/* ---- All experiment functions (defined in main.c) ---- */
extern void Exp1_LED_Flowing(void);
extern void Exp2_KeyPolling(void);
extern void Exp3_LightSensor(void);
extern void Exp4_USART1_Echo(void);
extern void Exp5_TrafficLight(void);
extern void Exp6_LCD12864(void);
extern void Exp7_CH451(void);
extern void Exp8_IndependentKeys(void);
extern void Exp9_DIPSwitch(void);
extern void Exp10_Relay(void);
extern void Exp11_DHT11(void);
extern void Exp12_Thermistor(void);
extern void Exp13_DS18B20(void);
extern void Exp14_DA(void);
extern void Exp15_RS232(void);
extern void Exp16_RS485(void);
extern void Exp17_Infrared(void);
extern void Exp18_Encoder(void);
extern void Exp18_StepperMotor(void);
extern void Exp19_DCMotorPWM(void);
extern void Exp20_Servo(void);

/* ---- Run one experiment by mode number ---- */
static void RunExperiment(uint8_t mode)
{
        switch (mode) {
        case  0: Exp6_LCD12864();      break;
        case  1: Exp1_LED_Flowing();   break;
        case  2: Exp2_KeyPolling();    break;
        case  3: Exp8_IndependentKeys(); break;
        case  4: Exp9_DIPSwitch();     break;
        case  5: Exp3_LightSensor();   break;
        case  6: Exp12_Thermistor();   break;
        case  7: Exp11_DHT11();        break;
        case  8: Exp13_DS18B20();      break;
        case  9: Exp4_USART1_Echo();   break;
        case 10: Exp15_RS232();        break;
        case 11: Exp16_RS485();        break;
        case 12: Exp17_Infrared();     break;
        case 13: Exp5_TrafficLight();  break;
        case 14: Exp7_CH451();         break;
        case 15: Exp10_Relay();        break;
        case 16: Exp14_DA();           break;
        case 17: Exp18_Encoder();      break;
        case 18: Exp19_DCMotorPWM();   break;
        case 19: Exp20_Servo();        break;
        case 20: Exp18_StepperMotor(); break;
        default: break;
    }
}

/* ---- ExperimentTask: blocks on queue, runs one experiment at a time ---- */
void vExperimentTask(void *pvParameters)
{
    uint8_t mode;

    (void)pvParameters;

    for (;;) {
        if (xQueueReceive(xModeQueue, &mode, portMAX_DELAY) == pdTRUE) {
            stop_requested = 0;
            printf("--> [RTOS] Switching to mode %d\r\n", mode);
            RunExperiment(mode);
        }
    }
}

/* ---- ControlTask: polls WKUP, sends mode change via queue ---- */
void vControlTask(void *pvParameters)
{
    uint32_t exit_count = 0;
    uint8_t  next_mode  = 0;

    (void)pvParameters;

    /* Signal delay_ms that the scheduler is now running */
    scheduler_started = 1;

    /* Bootstrap: send mode 0 so ExperimentTask starts immediately */
    xQueueSend(xModeQueue, &next_mode, portMAX_DELAY);
    next_mode = 1;

    for (;;) {
        if (WKUP_IsPressed()) {
            if (++exit_count > 10) {           /* 10 x 50ms = 500ms long-press */
                stop_requested = 1;
                xQueueSend(xModeQueue, &next_mode, portMAX_DELAY);
                if (++next_mode >= 21) next_mode = 0;
                exit_count = 0;
            }
        } else {
            exit_count = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ---- Create tasks and queue ---- */
void freertos_init(void)
{
    /* Create queue: 1 slot, uint8_t items */
    xModeQueue = xQueueCreate(1, sizeof(uint8_t));

    /* ControlTask: highest user priority (4), 256-word stack */
    xTaskCreate(vControlTask, "Control", 256, NULL, 4, &xControlTaskHandle);

    /* ExperimentTask: priority 3, 512-word stack for deep call chains */
    xTaskCreate(vExperimentTask, "Experiment", 512, NULL, 3, &xExperimentTaskHandle);
}
