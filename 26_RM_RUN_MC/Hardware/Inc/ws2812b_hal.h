//
// Created by Hanjiang
//

#ifndef MASTERCONTROL_WS2812B_HAL_H
#define MASTERCONTROL_WS2812B_HAL_H

#include "bitmap.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_tim.h"
#include "tim.h"
//#include "stm32f1xx_hal_dma.h"
//#include "stm32f1xx_hal_dma_ex.h"

/* Conf ------------------------------------------------------------------*/
#define WS2812B_USE_GAMMA_CORRECTION
#define WS2812B_USE_PRECALCULATED_GAMMA_TABLE

#define WS2812B_BUFFER_SIZE     60
#define WS2812B_START_SIZE      2

#define WS2812B_PULSE_HIGH      21
#define WS2812B_PULSE_LOW       9


/* Inf ------------------------------------------------------------------*/
int ws2812b_IsReady(void);

void ws2812b_M4H1_SendRGB(RGB_t *rgb, unsigned count);
void ws2812b_M4H1_SendHSV(HSV_t *hsv, unsigned count);

void ws2812b_M4H2_SendRGB(RGB_t *rgb, unsigned count);
void ws2812b_M4H2_SendHSV(HSV_t *hsv, unsigned count);

void ws2812b_M3H1_SendRGB(RGB_t *rgb, unsigned count);
void ws2812b_M3H1_SendHSV(HSV_t *hsv, unsigned count);

void ws2812b_M1H1_SendRGB(RGB_t *rgb, unsigned count);
void ws2812b_M1H1_SendHSV(HSV_t *hsv, unsigned count);

void ws2812b_M2H1_SendRGB(RGB_t *rgb, unsigned count);
void ws2812b_M2H1_SendHSV(HSV_t *hsv, unsigned count);

#endif //MASTERCONTROL_WS2812B_HAL_H
