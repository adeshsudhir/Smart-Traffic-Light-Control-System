#ifndef __HCSR04_H__
#define __HCSR04_H__

#include "stm32f4xx_hal.h"

typedef struct {
    TIM_HandleTypeDef *htim;
    GPIO_TypeDef *trig_port;
    uint16_t trig_pin;
    GPIO_TypeDef *echo_port;
    uint16_t echo_pin;
    volatile uint32_t start_time;
    volatile uint32_t end_time;
    volatile uint8_t  captured;
} HCSR04_t;

void HCSR04_InitSensor(HCSR04_t *sensor,
                       TIM_HandleTypeDef *htim,
                       GPIO_TypeDef *trig_port,
					   uint16_t trig_pin,
                       GPIO_TypeDef *echo_port,
					   uint16_t echo_pin
);

void HCSR04_Trigger(HCSR04_t *sensor);
float HCSR04_Read(HCSR04_t *sensor);
void HCSR04_EchoCallback(uint16_t GPIO_Pin);

extern HCSR04_t hcsr04_sensors[];
extern uint8_t hcsr04_sensor_count;

#endif
