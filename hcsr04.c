#include "hcsr04.h"

HCSR04_t hcsr04_sensors[4]; // Define max 4 sensors
uint8_t hcsr04_sensor_count = 0;

void HCSR04_InitSensor(HCSR04_t *sensor,
                       TIM_HandleTypeDef *htim,
                       GPIO_TypeDef *trig_port, uint16_t trig_pin,
                       GPIO_TypeDef *echo_port, uint16_t echo_pin)
{
    sensor->htim = htim;
    sensor->trig_port = trig_port;
    sensor->trig_pin = trig_pin;
    sensor->echo_port = echo_port;
    sensor->echo_pin = echo_pin;
    sensor->captured = 0;

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // TRIG output
    GPIO_InitStruct.Pin = trig_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(trig_port, &GPIO_InitStruct);

    // ECHO input with interrupt
    GPIO_InitStruct.Pin = echo_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(echo_port, &GPIO_InitStruct);

    hcsr04_sensors[hcsr04_sensor_count] = *sensor;
    hcsr04_sensor_count++;
}

void HCSR04_Trigger(HCSR04_t *sensor)
{
    HAL_GPIO_WritePin(sensor->trig_port, sensor->trig_pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(sensor->trig_port, sensor->trig_pin, GPIO_PIN_SET);
    HAL_Delay(1); // >=10us
    HAL_GPIO_WritePin(sensor->trig_port, sensor->trig_pin, GPIO_PIN_RESET);
}

float HCSR04_Read(HCSR04_t *sensor)
{
    sensor->captured = 0;
    sensor->start_time = 0;
    sensor->end_time = 0;

    HCSR04_Trigger(sensor);

    uint32_t timeout = HAL_GetTick() + 50;
    while (!sensor->captured && HAL_GetTick() < timeout);

    if (!sensor->captured)
        return -1; // timeout

    uint32_t pulse_duration = sensor->end_time - sensor->start_time;
    return (pulse_duration * 0.0343f) / 2.0f;
}

// Called from HAL_GPIO_EXTI_Callback
void HCSR04_EchoCallback(uint16_t GPIO_Pin)
{
    for (int i = 0; i < hcsr04_sensor_count; i++)
    {
        HCSR04_t *s = &hcsr04_sensors[i];
        if (s->echo_pin == GPIO_Pin)
        {
            if (HAL_GPIO_ReadPin(s->echo_port, s->echo_pin) == GPIO_PIN_SET)
            {
                __HAL_TIM_SET_COUNTER(s->htim, 0);
                HAL_TIM_Base_Start(s->htim);
                s->start_time = 0;
            }
            else
            {
                s->end_time = __HAL_TIM_GET_COUNTER(s->htim);
                HAL_TIM_Base_Stop(s->htim);
                s->captured = 1;
            }
        }
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
   HCSR04_EchoCallback(GPIO_Pin);
}
