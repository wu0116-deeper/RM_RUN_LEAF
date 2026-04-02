// The MIT License (MIT)
//
// Copyright (c) 2015 Aleksandr Aleshin <silencer@quadrius.net>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/* Includes ------------------------------------------------------------------*/

#include <stdint.h>
#include <string.h>

#include "ws2812b_hal.h"


//------------------------------------------------------------
// Internal
//------------------------------------------------------------

#define MIN(a, b)   ({ typeof(a) a1 = a; typeof(b) b1 = b; a1 < b1 ? a1 : b1; })

#if defined(__ICCARM__)
__packed struct PWM
#else
struct __attribute__((packed)) PWM
#endif
{
    uint16_t g[8], r[8], b[8];
};

typedef struct PWM PWM_t;
typedef void (SrcFilter_t)(void **, PWM_t **, unsigned *, unsigned);

#ifdef WS2812B_USE_GAMMA_CORRECTION
#ifdef WS2812B_USE_PRECALCULATED_GAMMA_TABLE
static const uint8_t LEDGammaTable[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    10, 11, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21,
    22, 23, 23, 24, 24, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32, 32, 33, 34, 35, 35, 36, 37, 38,
    38, 39, 40, 41, 42, 42, 43, 44, 45, 46, 47, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58,
    59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 84,
    85, 86, 87, 88, 89, 91, 92, 93, 94, 95, 97, 98, 99, 100, 102, 103, 104, 105, 107, 108, 109, 111,
    112, 113, 115, 116, 117, 119, 120, 121, 123, 124, 126, 127, 128, 130, 131, 133, 134, 136, 137,
    139, 140, 142, 143, 145, 146, 148, 149, 151, 152, 154, 155, 157, 158, 160, 162, 163, 165, 166,
    168, 170, 171, 173, 175, 176, 178, 180, 181, 183, 185, 186, 188, 190, 192, 193, 195, 197, 199,
    200, 202, 204, 206, 207, 209, 211, 213, 215, 217, 218, 220, 222, 224, 226, 228, 230, 232, 233,
    235, 237, 239, 241, 243, 245, 247, 249, 251, 253, 255 };
#endif
#endif

static inline uint8_t LEDGamma(uint8_t v)
{
#ifdef WS2812B_USE_GAMMA_CORRECTION
    #ifdef WS2812B_USE_PRECALCULATED_GAMMA_TABLE
    return LEDGammaTable[v];
#else
    return (v * v + v) >> 8;
#endif
#else
    return v;
#endif
}

static volatile int DMABusy;

static PWM_t DMABuffer[WS2812B_BUFFER_SIZE];

static SrcFilter_t *DMAFilter;
static void *DMASrc;
static unsigned DMACount;


/*void Cpcallback(DMA_HandleTypeDef *hdma);*/

/*HAL_DMA_RegisterCallback(&hdma,HAL_DMA_XFER_CPLT_CB_ID,Cpcallback);*/

static void SrcFilterNull(void **src, PWM_t **pwm, unsigned *count, unsigned size)
{
    memset(*pwm, 0, size * sizeof(PWM_t));
    *pwm += size;
}

static void RGB2PWM(RGB_t *rgb, PWM_t *pwm)
{
    uint8_t r = LEDGamma(rgb->r);
    uint8_t g = LEDGamma(rgb->g);
    uint8_t b = LEDGamma(rgb->b);

    uint8_t mask = 128;

    int i;
    for (i = 0; i < 8; i++)
    {
        pwm->r[i] = r & mask ? WS2812B_PULSE_HIGH : WS2812B_PULSE_LOW;
        pwm->g[i] = g & mask ? WS2812B_PULSE_HIGH : WS2812B_PULSE_LOW;
        pwm->b[i] = b & mask ? WS2812B_PULSE_HIGH : WS2812B_PULSE_LOW;

        mask >>= 1;
    }
}

static void SrcFilterRGB(void **src, PWM_t **pwm, unsigned *count, unsigned size)
{
    RGB_t *rgb = *src;
    PWM_t *p = *pwm;

    *count -= size;

    while (size--)
    {
        RGB2PWM(rgb++, p++);
    }

    *src = rgb;
    *pwm = p;
}

static void SrcFilterHSV(void **src, PWM_t **pwm, unsigned *count, unsigned size)
{
    HSV_t *hsv = *src;
    PWM_t *p = *pwm;

    *count -= size;
    while (size--)
    {
        RGB_t rgb;

        HSV2RGB(hsv++, &rgb);
        RGB2PWM(&rgb, p++);
    }

    *src = hsv;
    *pwm = p;
}

static void DMA_CH2_Send(SrcFilter_t *filter, void *src, unsigned count) {
    if (!DMABusy) {
        DMABusy = 1;

        DMAFilter = filter;
        DMASrc = src;
        DMACount = count;

        PWM_t *pwm = DMABuffer;
        PWM_t *end = &DMABuffer[WS2812B_BUFFER_SIZE];

        // Start sequence
        SrcFilterNull(NULL, &pwm, NULL, WS2812B_START_SIZE);

        // RGB PWM data
        DMAFilter(&DMASrc, &pwm, &DMACount, MIN(DMACount, end - pwm));

        // Rest of buffer
        if (pwm < end)
            SrcFilterNull(NULL, &pwm, NULL, end - pwm);

        // Start transfer
        HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)DMABuffer,
                              sizeof(DMABuffer) / sizeof(uint16_t));//right

        //HAL_TIM_PWM_Start_DMA(&htim4, TIM_CHANNEL_1, &DMABuffer[WS2812B_BUFFER_SIZE],
        //                      DMACount*24);

    }
}

static void DMA_CH2_SendNext(PWM_t *pwm, PWM_t *end)
{
    if (!DMAFilter)
    {
        // Stop transfer
        //TIM_Cmd(WS2812B_TIM, DISABLE);

        HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);


        DMABusy = 0;
    }
    else if (!DMACount)
    {
        // Rest of buffer
        SrcFilterNull(NULL, &pwm, NULL, end - pwm);

        DMAFilter = NULL;
    }
    else
    {
        // RGB PWM data
        DMAFilter(&DMASrc, &pwm, &DMACount, MIN(DMACount, end - pwm));

        // Rest of buffer
        if (pwm < end)
            SrcFilterNull(NULL, &pwm, NULL, end - pwm);
    }
}


//--------------------------------------------------------------

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim)
{
    if(htim == &htim4 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        //DMA_CH1_SendNext(DMABuffer, &DMABuffer[WS2812B_BUFFER_SIZE / 2]);
    }
    else if(htim == &htim4 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
    {
        //DMA_CH4_SendNext(DMABuffer, &DMABuffer[WS2812B_BUFFER_SIZE / 2]);
    }
    else if(htim == &htim1 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        DMA_CH2_SendNext(DMABuffer, &DMABuffer[WS2812B_BUFFER_SIZE / 2]);
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if(htim == &htim4 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        //DMA_CH1_SendNext(&DMABuffer[WS2812B_BUFFER_SIZE / 2],&DMABuffer[WS2812B_BUFFER_SIZE]);
    }
    else if(htim == &htim4 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
    {
        //DMA_CH4_SendNext(&DMABuffer[WS2812B_BUFFER_SIZE / 2],&DMABuffer[WS2812B_BUFFER_SIZE]);
    }
    else if(htim == &htim1 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        DMA_CH2_SendNext(&DMABuffer[WS2812B_BUFFER_SIZE / 2],&DMABuffer[WS2812B_BUFFER_SIZE]);
    }
}



//------------------------------------------------------------
// Interface
//------------------------------------------------------------

inline int ws2812b_IsReady(void)
{
    return !DMABusy;
}



void ws2812b_M1H1_SendRGB(RGB_t *rgb, unsigned count)//PA8
{
    DMA_CH2_Send(&SrcFilterRGB, rgb, count);
}

void ws2812b_M1H1_SendHSV(HSV_t *hsv, unsigned count)//PA8
{
    DMA_CH2_Send(&SrcFilterHSV, hsv, count);
}