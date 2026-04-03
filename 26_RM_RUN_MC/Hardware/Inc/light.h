//
// Created by Lenovo on 2024/1/23.
//

#ifndef MASTERCONTROL_LIGHT_H
#define MASTERCONTROL_LIGHT_H

#include "ws2812b_hal.h"

typedef enum { LEAF_OFF, LEAF_STRIKING, LEAF_STRUCK} LED_Leaf_Mode_t; //对于单片LED的状态
//              关闭         打击中           击中

typedef enum { LEAF_1, LEAF_2, LEAF_3, LEAF_4, LEAF_5} Leaf_NUM_t;
//叶片名

typedef enum { RedState, BlueState, DebugState} LED_State_t; //颜色

static const RGB_t R_red = {255, 0, 0};  //R标的颜色
static const RGB_t R_blue = {0, 0, 255};   //R标的颜色

void LED_Init();
void LED_Padding2Update();

#endif //MASTERCONTROL_LIGHT_H
