#ifndef MASTERCONTROL_WS2812B_HAL_H
#define MASTERCONTROL_WS2812B_HAL_H

#include "stm32f1xx_hal.h"

/* Conf ------------------------------------------------------------------*/
#define WS2812B_USE_GAMMA_CORRECTION
#define WS2812B_USE_PRECALCULATED_GAMMA_TABLE

#define WS2812B_BUFFER_SIZE     60
#define WS2812B_START_SIZE      2

#define WS2812B_PULSE_HIGH      21
#define WS2812B_PULSE_LOW       9

struct RGB
{
    uint8_t r, g, b;
};
typedef struct RGB RGB_t;

/* Inf ------------------------------------------------------------------*/
void ws2812b_Init(void);
int  ws2812b_IsReady(void);

void ws2812b_M1H1_SendRGB(RGB_t *rgb, unsigned count); // PA8 - TIM1_CH1
void ws2812b_M4H1_SendRGB(RGB_t *rgb, unsigned count); // PB6 - TIM4_CH1
void ws2812b_M4H2_SendRGB(RGB_t *rgb, unsigned count); // PB7 - TIM4_CH2
void ws2812b_M3H1_SendRGB(RGB_t *rgb, unsigned count); // PB4 - TIM3_CH1
void ws2812b_M2H2_SendRGB(RGB_t *rgb, unsigned count); // PB3 - TIM2_CH2

#endif //MASTERCONTROL_WS2812B_HAL_H
