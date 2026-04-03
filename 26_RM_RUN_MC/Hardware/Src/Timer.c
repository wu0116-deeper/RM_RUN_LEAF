//
// Created by Lenovo on 2024/1/20.
//
#include "stm32f1xx_hal.h"
#include "Timer.h"
#include "usart.h"

uint32_t systime_ms = 0;
uint16_t overflow_count = 0;
uint16_t cnt = 0;	//存CNT寄存器的初值

extern uint8_t timeout;
extern uint8_t debug;
extern uint8_t post_hit_window; // 由 light.c 定义，1=当前处于击中后1s窗口
uint8_t post_hit_timeout = 0;   // 1s窗口到期标志，由本文件置1，light.c清零
/*extern uint8_t refresh_rectangle;*/
extern TIM_HandleTypeDef htim4;


void Timer_reset(void)
{
    systime_ms = 0;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == (&htim4))
    {
        if(debug != 1)
        {
            systime_ms++;
        }

        if (post_hit_window == 1 && systime_ms == 100)  // 1s：击中后等待第二个扇叶的窗口到期
        {
            Timer_reset();
            post_hit_window = 0;
            post_hit_timeout = 1;
        }
        else if (post_hit_window == 0 && systime_ms == 250)  // 2.5s：未击中超时
        {
            Timer_reset();
            timeout = 1;
        }

        overflow_count++;
    }

}

void TIM4_StartCounter(void)
{
    overflow_count = 0;
    cnt = __HAL_TIM_GET_COUNTER(&htim4);
    /*printf("value: %lu\n",cnt);*/
}

uint16_t TIM4_GetCounter(void)
{
    /*printf("value: %lu\r\n",(__HAL_TIM_GET_COUNTER(&htim4) - cnt));
    printf("overflow: %lu\r\n",overflow_count);*/
    return overflow_count * 10000 + __HAL_TIM_GET_COUNTER(&htim4) - cnt;	//一般情况下，不会就不需要overflow(除非测一次影子时间时)
}

