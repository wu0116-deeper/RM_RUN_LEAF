#include "light.h"
#include "main.h"
#include "pattern.h"
#include "struck.h"
#include "deliver.h"

// ========== 灯珠数量测试模式 ==========
// 修改这个数字来测试不同数量的灯珠，全部通过PA8发送
#define LED_TEST_MODE       0       // 1=测试模式, 0=正常模式
#define LED_TEST_COUNT      300// 测试灯珠数量，修改这个值
// =====================================

LED_State_t ledstate = RedState; // 默认状态为红色
LED_Leaf_Mode_t leafmode = LEAF_OFF; // 默认叶片状态为关闭
LED_Leaf_Mode_t prev_leafmode = LEAF_STRIKING; // 调试
GameMode_t gamemode = MODE_DaFu; // 默认游戏模式为大夫


static RGB_t leds_demo[LED_DEMO_NUM];
static RGB_t leds_target[LED_TARGET_NUM];
static RGB_t leds_rectangle_arrows[LED_RECTANGLE_ARROWS_NUM];
static RGB_t leds_strip[LED_STRIP_NUM];
static RGB_t leds_framedata1[LED_FRAMEDATA1_NUM];
static RGB_t leds_framedata2[LED_FRAMEDATA2_NUM];

RGB_t current_color = {0, 0, 0};
RGB_t debug_color = {50, 50, 50}; // 调试模式下的颜色
uint8_t debug = 0; // 调试模式标志
uint8_t refresh_rectangle = 0;
uint8_t activated_progress = 0;

static volatile uint8_t hit_latched = 0;

uint8_t led_rx_arr[8];
uint8_t received_round_id = 0;  // 主控下发的本轮令牌，回传给主控用于去重
uint16_t kopp;

const uint8_t* const TARGET_RINGS[] = {
    NULL,                  // index 0 占位
    TARGET_STRUCK_ring1,
    TARGET_STRUCK_ring2,
    TARGET_STRUCK_ring3,
    TARGET_STRUCK_ring4,
    TARGET_STRUCK_ring5,
    TARGET_STRUCK_ring6,
    TARGET_STRUCK_ring7,
    TARGET_STRUCK_ring8,
    TARGET_STRUCK_ring9
};

//  void Target_FullOn_Test(void)
// {
//     static const RGB_t red_levels[3] = {
//         {20, 0, 0},
//         {80, 0, 0},
//         {180, 0, 0}
//     };
//     static uint8_t level_idx = 0;
//     static uint32_t last_switch_tick = 0;
//
//     if ((HAL_GetTick() - last_switch_tick) >= 1500U)
//     {
//         level_idx = (uint8_t)((level_idx + 1U) % 3U);
//         last_switch_tick = HAL_GetTick();
//     }
//
//     for (int i = 0; i < LED_TARGET_NUM; i++)
//     {
//         leds_target[i] = red_levels[level_idx];
//     }
//
//     while (!ws2812b_IsReady());
//     ws2812b_M1H1_SendRGB(leds_target, LED_TARGET_NUM);
// }

void Light_Init(LED_State_t state)
{
    ledstate = state;
    current_color = (ledstate == RedState ? red : blue);
    ws2812b_Init();  // 初始化ws2812b驱动
    
    // 清零所有LED数组
    for(int i = 0; i < LED_TARGET_NUM; i++) leds_target[i] = off;
    for(int i = 0; i < LED_RECTANGLE_ARROWS_NUM; i++) leds_rectangle_arrows[i] = off;
    for(int i = 0; i < LED_STRIP_NUM; i++) leds_strip[i] = off;
    for(int i = 0; i < LED_FRAMEDATA1_NUM; i++) leds_framedata1[i] = off;
    for(int i = 0; i < LED_FRAMEDATA2_NUM; i++) leds_framedata2[i] = off;
    
    // 等待DMA就绪后发送一次关灯数据
    while(!ws2812b_IsReady());
    ws2812b_M3H1_SendRGB(leds_rectangle_arrows, LED_RECTANGLE_ARROWS_NUM);
    while(!ws2812b_IsReady());
    ws2812b_M1H1_SendRGB(leds_target, LED_TARGET_NUM);
    while(!ws2812b_IsReady());
    ws2812b_M2H2_SendRGB(leds_strip, LED_STRIP_NUM);
    while(!ws2812b_IsReady());
    ws2812b_M4H1_SendRGB(leds_framedata1, LED_FRAMEDATA1_NUM);
    while(!ws2812b_IsReady());
    ws2812b_M4H2_SendRGB(leds_framedata2, LED_FRAMEDATA2_NUM);
}


void led_debug(LED_Leaf_Mode_t leafmode, RGB_t *dst)
{
    debug = 1;
    current_color = debug_color;
    for(int i = 0; i < LED_DEMO_NUM; i++){
        dst[i] = current_color;
    }
}

void demoData(LED_Leaf_Mode_t leafmode, RGB_t *dst)
{
    if(debug == 1)
    {
        current_color = debug_color;
        for(int i = 0; i < LED_DEMO_NUM; i++){
            dst[i] = current_color;
        }
    }
    else
    {
        switch (leafmode)
        {
        case LEAF_OFF:
            for(int i = 0; i < LED_DEMO_NUM; i++){
                dst[i] = off;
            }
            break;
        case LEAF_STRIKING:
            for(int i = 0; i < LED_DEMO_NUM; i++){
                if(DEMO_STRIKING[i]){
                    dst[i] = current_color;
                }
            }
            break;
        case LEAF_STRUCK:
            // 先清除所有LED，确保旧环数效果被清零
            for (int i = 0; i < LED_DEMO_NUM; i++) {
                dst[i] = off;
            }
            // 再设置当前环数的效果
            if (struck_ring == 9)
            {
                for (int i = 0; i < LED_DEMO_NUM; i++) {
                    if (DEMO_STRUCK_ring9[i]) {
                        dst[i] = current_color;
                    }
                }
            }
            else if (struck_ring == 8)
            {
                for (int i = 0; i < LED_DEMO_NUM; i++)
                {
                    if (DEMO_STRUCK_ring8[i])
                    {
                        dst[i] = current_color;
                    }
                }
            }
            break;
        }
    }
}
    //     // 默认所有灯灭
    //     for(int i = 0; i < LED_DEMO_NUM; i++){
    //         dst[i] = off;
    //     }
    // }
    // // key9 (PB1) 按下 → DEMO_STRUCK_ring9: 点亮后9颗 (index 12-20)
    // if(pb1 == GPIO_PIN_RESET){
    //     for(int i = 0; i < LED_DEMO_NUM; i++){
    //         if(DEMO_STRUCK_ring9[i]){
    //             dst[i] = current_color;
    //         }
    //     }
    // }
    //
    // // key8 (PB0) 按下 → DEMO_STRUCK_ring8: 点亮前12颗 (index 0-11)
    // if(pb0 == GPIO_PIN_RESET){
    //     for(int i = 0; i < LED_DEMO_NUM; i++){
    //         if(DEMO_STRUCK_ring8[i]){
    //             dst[i] = current_color;
    //         }
    //     }
    // }


void TargetData(LED_Leaf_Mode_t leafmode, RGB_t *dst)
{
    if(debug == 1){
        current_color = debug_color;
        for(int i = 0; i < LED_TARGET_NUM; i++){
            dst[i] = current_color;
        }
    }else{
        switch (leafmode) {
            case LEAF_OFF:
                for(int i = 0; i < LED_TARGET_NUM; i++){
                    dst[i] = off;
                }
                break;
            case LEAF_STRIKING:
                for(int i = 0; i < LED_TARGET_NUM; i++){
                    if(TARGET_STRIKING[i]){
                        dst[i] = current_color;
                    }else{
                        dst[i] = off;
                    }
                }
                break;
            case LEAF_STRUCK:
                if (struck_ring >= 1 && struck_ring <= 9) {
                    
                    const uint8_t* current_target = TARGET_RINGS[struck_ring];
                    
                    for (int i = 0; i < LED_TARGET_NUM; i++) {
                        
                        dst[i] = current_target[i] ? current_color : off;
                    }
                }
                break; 
        }
    }
}

void RectangleArrowsData(LED_Leaf_Mode_t leafmode, RGB_t *dst)
{
    if(debug == 1){
        current_color = debug_color;
        for(int i = 0; i < LED_RECTANGLE_ARROWS_NUM; i++){
            dst[i] = current_color;
        }
    }else{
        static uint8_t count = 4;// 4帧切换一次箭头位置
        static uint8_t fut = 4;// 切换帧数倒计时
        if(gamemode == MODE_XiaoFu){
            switch (leafmode) {
                case LEAF_OFF:
                    for(int i = 0; i < LED_RECTANGLE_ARROWS_NUM; i++){
                    dst[i] = off;
                }
                    break;
                case LEAF_STRIKING:
                    if (refresh_rectangle == 1) { fut--; if (fut == 0) { count--; fut = 4; } }
                    if (count > 0) refresh_rectangle = 0;
                    else if (count == 0) { count = 4; refresh_rectangle = 0; }
                    for (uint16_t i = 0; i < 256; i++)
                    {
                        if ((i + count * 8 < 256) && RECTANGLE_ARROWS[i + count * 8])
                        dst[i] = current_color;
                        else if ((i + count * 8 >= 256) && RECTANGLE_ARROWS[i + count * 8 - 256])
                        dst[i] = current_color;
                        else
                        dst[i] = off;
                    }
                    break;
                case LEAF_STRUCK:
                for(int i = 0; i < LED_RECTANGLE_ARROWS_NUM; i++){
                    dst[i] = current_color;
                }
                    break;
            }
        }else if(gamemode == MODE_DaFu){
            if (leafmode == LEAF_STRIKING)
            {
                if (refresh_rectangle == 1) { fut--; if (fut == 0) { count--; fut = 4; } }
                if (count > 0) refresh_rectangle = 0;
                else if (count == 0) { count = 4; refresh_rectangle = 0; }
                for (uint16_t i = 0; i < 256; i++)
                {
                    if ((i + count * 8 < 256) && RECTANGLE_ARROWS[i + count * 8])
                    dst[i] = current_color;
                    else if ((i + count * 8 >= 256) && RECTANGLE_ARROWS[i + count * 8 - 256])
                    dst[i] = current_color;
                    else
                    dst[i] = off;
                }
            }   
            else
            {
            static const uint16_t group_end[5] = {48, 96, 144, 192, 256};
            uint16_t lit_boundary = 0;
            if (activated_progress >= 1 && activated_progress <= 5)
                lit_boundary = group_end[activated_progress - 1];
            for (uint16_t i = 0; i < 256; i++)
                dst[i] = (i < lit_boundary) ? current_color : off;
            }
        }         
    }
}

void StripData(LED_Leaf_Mode_t leafmode, RGB_t *dst)
{
    if(debug == 1){
        current_color = debug_color;
        for(int i = 0; i < LED_STRIP_NUM; i++){
            dst[i] = current_color;
        }
    }else {
        if(gamemode == MODE_DaFu){
            if (leafmode == LEAF_STRIKING) {
                for (uint8_t i = 0; i < LED_STRIP_NUM; i++) {
                dst[i] = off;
                }
            }else{
                // 进度条分组区间
                static const uint8_t group_ranges[5][4] = {
                    {18, 33, 0, 0},      // 组1: [18,33]
                    {14, 17, 34, 37},    // 组2: [14,17] [34,37]
                    {10, 13, 38, 41},    // 组3: [10,13] [38,41]
                    {6, 9, 42, 45},      // 组4: [6,9] [42,45]
                    {1, 5, 46, 50}       // 组5: [1,5] [46,50]
                    };
                // 先全部熄灭
                for (uint8_t i = 0; i < LED_STRIP_NUM; i++) {
                    dst[i] = off;
                }
                // 根据进度点亮
                uint8_t progress = activated_progress;
                if (progress > 5) progress = 5;
                for (uint8_t g = 0; g < progress; g++) {
                    // 每组最多两个区间
                    for (uint8_t r = 0; r < 2; r++) {
                        uint8_t start = group_ranges[g][r * 2];
                        uint8_t end = group_ranges[g][r * 2 + 1];
                        if (start == 0 && end == 0) continue; // 跳过无效区间
                            for (uint8_t i = start; i <= end && i <= 49; i++) {
                            dst[i] = current_color;
                        }
                    }
                }
            }
        }
        else if(gamemode == MODE_XiaoFu){
            if (leafmode == LEAF_STRUCK) {
                for (uint8_t i = 0; i < LED_STRIP_NUM; i++) {
                    dst[i] = current_color;
                }
            }else{
                for (uint8_t i = 0; i < LED_STRIP_NUM; i++) {
                    dst[i] = off;
                }
            }
        }
    }
}

void LED_PackFrameData1(LED_Leaf_Mode_t leafmode, RGB_t *dst)
{
    /* 前框架仅在大符完全激活（5 组全部命中）时点亮 */
    if (activated_progress >= 5)
    {
        for (uint8_t i = 0; i < LED_FRAMEDATA1_NUM ; i++)
            dst[i] = current_color;
    }
    else
    {
        for (uint8_t i = 0; i < LED_FRAMEDATA1_NUM ; i++)
            dst[i] = off;
    }
}

void LED_PackFrameData2(LED_Leaf_Mode_t leafmode, RGB_t *dst)
{
    /* 后框架仅在大符完全激活（5 组全部命中）时点亮 */
    if (activated_progress >= 5)
    {
        for (uint8_t i = 0; i < LED_FRAMEDATA2_NUM ; i++)
            dst[i] = current_color;
    }
    else
    {
        for (uint8_t i = 0; i < LED_FRAMEDATA2_NUM ; i++)
            dst[i] = off;
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];

    kopp++;  // 接收计数器，用于调试
    
    // PA7翻转表示收到CAN消息（调试指示）
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_10);
    
    // 从CAN FIFO0获取消息 
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data);
    
    // 复制所有8字节数据到全局缓冲区
    for (int i = 0; i < 8; i++)
    {
        led_rx_arr[i] = rx_data[i];
    }
    // byte[4] = 主控下发的 round_id 令牌，回传给主控用于去重
    received_round_id = rx_data[4];
}

void check_LED_Status(void)
{
    
    // ============ 更新颜色阵营 ============
    if(led_rx_arr[0] == 0)
    {
        ledstate = RedState;           // 红方阵营
        current_color = red;
    }
    else if(led_rx_arr[0] == 1)
    {
        ledstate = BlueState;          // 蓝方阵营
        current_color = blue;
    }
    
    // ============ 更新激活进度（主控下发，用于灯带中部进度条） ============
    activated_progress = led_rx_arr[3];

    /*
        // led_rx_arr[0] = current_color;
        // lea_rx_arr[1] = leafmode;
        // led_rx_arr[2] = struct_ring;
        // led_rx_arr[3] = activated_group
        // led_rx_arr[4] = received_round_id;
    */ 

    uint8_t new_mode = led_rx_arr[1];
    if(new_mode == LEAF_OFF)
    {
        leafmode = LEAF_OFF;            // 关闭状态（作为下一轮复位信号）
        struck_ring = 0;
        hit_latched = 0U;
    }
    else if(new_mode == LEAF_STRIKING)
    {
        if (hit_latched == 0U)
        {
            leafmode = LEAF_STRIKING;   // 击打状态（等待主控确认）
        }
    }
    else if(new_mode == LEAF_STRUCK)
    {
        leafmode = LEAF_STRUCK;         // 被击中状态
        struck_ring = led_rx_arr[2]; // 读取环数
        hit_latched = 1U;
    }

    if(struck_flag == 1U)
    {
        if(leafmode == LEAF_STRIKING)
        {
            hit_latched = 1U;
            struck_flag = 1U;  // 恢复标志，供 send_strike_data() 使用
        }
        else
        {
            struck_flag = 0U;
        }
    }
}

static void check_LED_Status_Local(void)
{
    ReadStruck();

    if (struck_flag == 1U)
    {
        leafmode = LEAF_STRUCK;
        hit_latched = 1U;
    }
    else
    {
        // 松手后，无论之前的 struck_ring 值如何，都应该关闭灯
        leafmode = LEAF_STRIKING;
        hit_latched = 0U;
    }
}

void led_update(void){

#if TARGET_FULL_ON_TEST_MODE
    Target_FullOn_Test();
    return;
#endif

#if LED_TEST_MODE
    // ===== 测试模式：复用leds_target缓冲区 =====
    static const RGB_t white = {50, 50, 50};
    
    for(int i = 0; i < LED_TEST_COUNT && i < LED_TARGET_NUM; i++) {
        leds_target[i] = white;
    }
    
    while(!ws2812b_IsReady());
    ws2812b_M1H1_SendRGB(leds_target, LED_TEST_COUNT);  // PA8 发送
    // ===============================================
#else
    // ===== 正常模式 =====
#if (LEAF_CONTROL_MODE == LEAF_CONTROL_LOCAL)
    check_LED_Status_Local();
#else
    check_LED_Status();
    send_strike_data();
#endif

    activated_progress = 5;
    
    // 准备所有通道数据
    TargetData(leafmode, leds_target);
    RectangleArrowsData(leafmode, leds_rectangle_arrows);
    StripData(leafmode, leds_strip);
    LED_PackFrameData1(leafmode, leds_framedata1);
    LED_PackFrameData2(leafmode, leds_framedata2);

    // 分时发送：一个通道发完再发下一个（共享DMA缓冲区）
    while(!ws2812b_IsReady());
    ws2812b_M3H1_SendRGB(leds_rectangle_arrows, LED_RECTANGLE_ARROWS_NUM); // PB4
    
    while(!ws2812b_IsReady());
    ws2812b_M1H1_SendRGB(leds_target, LED_TARGET_NUM);           // PA8
    
    while(!ws2812b_IsReady());
    ws2812b_M2H2_SendRGB(leds_strip, LED_STRIP_NUM);             // PB3
    
    while(!ws2812b_IsReady());
    ws2812b_M4H1_SendRGB(leds_framedata1, LED_FRAMEDATA1_NUM);   // PB6
     
    while(!ws2812b_IsReady());
    ws2812b_M4H2_SendRGB(leds_framedata2, LED_FRAMEDATA2_NUM);   // PB7
#endif
}