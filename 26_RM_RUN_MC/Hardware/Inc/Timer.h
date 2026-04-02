//
// Created by Lenovo on 2024/1/20.
//

#ifndef MASTERCONTROL_TIMER_H
#define MASTERCONTROL_TIMER_H

/*void Timer_Init(void);*/
void Timer_reset(void);
void TIM4_StartCounter(void);
uint16_t TIM4_GetCounter(void);

extern uint8_t post_hit_timeout; // 击中后1s窗口超时标志（Timer.c中定义）

#endif //MASTERCONTROL_TIMER_H
