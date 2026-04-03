#include <stdint.h>
#include <string.h>

#include "tim.h"
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

//------------------------------------------------------------
// 共享DMA缓冲区（节省RAM）
//------------------------------------------------------------

static PWM_t DMABuffer[WS2812B_BUFFER_SIZE];  // 所有通道共享

// 当前活动通道状态
static volatile int      DMABusy = 0;
static SrcFilter_t      *DMAFilter = NULL;
static void             *DMASrc = NULL;
static unsigned          DMACount = 0;
static TIM_HandleTypeDef *DMACurrentTim = NULL;
static uint32_t          DMACurrentCh = 0;

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

//--------------------------------------------------------------
// DMA发送（共享缓冲区）
//--------------------------------------------------------------

static void DMA_Send(TIM_HandleTypeDef *tim, uint32_t timCh, SrcFilter_t *filter, void *src, unsigned count)
{
    if (DMABusy) return;  // 忙则跳过
    
    DMABusy = 1;
    DMAFilter = filter;
    DMASrc = src;
    DMACount = count;
    DMACurrentTim = tim;
    DMACurrentCh = timCh;
    
    PWM_t *pwm = DMABuffer;
    PWM_t *end = &DMABuffer[WS2812B_BUFFER_SIZE];
    
    // 起始序列（低电平）
    SrcFilterNull(NULL, &pwm, NULL, WS2812B_START_SIZE);
    
    // 填充RGB PWM数据
    DMAFilter(&DMASrc, &pwm, &DMACount, MIN(DMACount, (unsigned)(end - pwm)));
    
    // 剩余部分清零
    if (pwm < end)
        SrcFilterNull(NULL, &pwm, NULL, end - pwm);
    
    // 启动DMA传输
    HAL_TIM_PWM_Start_DMA(tim, timCh, (uint32_t *)DMABuffer,
                          sizeof(DMABuffer) / sizeof(uint16_t));
}

static void DMA_SendNext(PWM_t *pwm, PWM_t *end)
{
    if (!DMAFilter)
    {
        // 传输完成，停止DMA
        HAL_TIM_PWM_Stop_DMA(DMACurrentTim, DMACurrentCh);
        DMABusy = 0;
    }
    else if (!DMACount)
    {
        // 数据已全部填充，填零剩余部分
        SrcFilterNull(NULL, &pwm, NULL, end - pwm);
        DMAFilter = NULL;
    }
    else
    {
        // 继续填充RGB PWM数据
        DMAFilter(&DMASrc, &pwm, &DMACount, MIN(DMACount, (unsigned)(end - pwm)));
        
        // 剩余部分清零
        if (pwm < end)
            SrcFilterNull(NULL, &pwm, NULL, end - pwm);
    }
}

//--------------------------------------------------------------
// DMA回调（Circular模式双缓冲）
//--------------------------------------------------------------

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim)
{
    // DMA传输到一半，填充前半部分
    if (htim == DMACurrentTim)
    {
        DMA_SendNext(DMABuffer, &DMABuffer[WS2812B_BUFFER_SIZE / 2]);
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    // DMA传输完成，填充后半部分
    if (htim == DMACurrentTim)
    {
        DMA_SendNext(&DMABuffer[WS2812B_BUFFER_SIZE / 2], &DMABuffer[WS2812B_BUFFER_SIZE]);
    }
}

//------------------------------------------------------------
// Interface
//------------------------------------------------------------

void ws2812b_Init(void)
{
    memset(DMABuffer, 0, sizeof(DMABuffer));
    DMABusy = 0;
    DMAFilter = NULL;
}

int ws2812b_IsReady(void)
{
    return !DMABusy;
}

void ws2812b_M1H1_SendRGB(RGB_t *rgb, unsigned count) // PA8 - TIM1_CH1
{
    DMA_Send(&htim1, TIM_CHANNEL_1, &SrcFilterRGB, rgb, count);
}

void ws2812b_M4H1_SendRGB(RGB_t *rgb, unsigned count) // PB6 - TIM4_CH1
{
    DMA_Send(&htim4, TIM_CHANNEL_1, &SrcFilterRGB, rgb, count);
}

void ws2812b_M4H2_SendRGB(RGB_t *rgb, unsigned count) // PB7 - TIM4_CH2
{
    DMA_Send(&htim4, TIM_CHANNEL_2, &SrcFilterRGB, rgb, count);
}

void ws2812b_M3H1_SendRGB(RGB_t *rgb, unsigned count) // PB4 - TIM3_CH1
{
    DMA_Send(&htim3, TIM_CHANNEL_1, &SrcFilterRGB, rgb, count);
}

void ws2812b_M2H2_SendRGB(RGB_t *rgb, unsigned count) // PB3 - TIM2_CH2
{
    DMA_Send(&htim2, TIM_CHANNEL_2, &SrcFilterRGB, rgb, count);
}
