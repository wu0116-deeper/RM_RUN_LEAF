#ifndef STRUCK_H
#define STRUCK_H

#include <stdint.h>

extern uint8_t struck_ring; // 当前击打环数，供外部模块读取
void ReadStruck(void); // 读取击打环数的函数声明

extern volatile uint8_t struck_flag;
extern void send_strike_data(void); // 发送击打数据的函数声明
#endif  