//
// Created by Lenovo on 2024/1/23.
//

#include "stm32f1xx_hal.h"
#include <stdlib.h>
#include "rand_creator.h"
#include "usart.h"

extern TIM_HandleTypeDef htim3;
uint8_t last_rand1 = 9;
uint8_t last_rand2 = 9;

// 生成随机数的动作
uint8_t rand_create()
{
    uint8_t rand_value;
    srand(__HAL_TIM_GET_COUNTER(&htim3)*100);
    rand_value = rand() % (RANDOM_MAX + 1 - RANDOM_MIN) + RANDOM_MIN;
    return rand_value;
}

// 生成两个不同的随机数，启动函数
void rand_check_two(uint8_t* leaf1, uint8_t* leaf2)
{
    *leaf1 = rand_create();
    HAL_Delay(1);
    *leaf2 = rand_create();
    
    // 确保两个都与上次的两个不同，且两个之间也不同
    while (*leaf1 == last_rand1 || *leaf1 == last_rand2)
    {
        *leaf1 = rand_create();
    }
    
    while (*leaf2 == last_rand1 || *leaf2 == last_rand2 || *leaf2 == *leaf1)
    {
        *leaf2 = rand_create();
    }
    
    last_rand1 = *leaf1;
    last_rand2 = *leaf2;
}
