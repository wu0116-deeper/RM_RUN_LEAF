//
// Created by Lenovo on 2024/1/23.
//

#include "stm32f1xx_hal.h"
#include "light.h"
#include "rand_creator.h"
#include "deliver.h"
#include "Timer.h"
#include "usart.h"


uint8_t debug = 0;
uint8_t timeout = 0;//超时标志

uint8_t total_struck; //总击打次数
uint8_t current_leaf;//当前扇叶

LED_State_t current_color = BlueState;//当前颜色
uint8_t leaf_ring_value[5] = {0, 0, 0, 0, 0}; //环数数组

LED_Leaf_Mode_t leafmode[5] = {LEAF_OFF, LEAF_OFF, LEAF_OFF, LEAF_OFF, LEAF_OFF};//当前状态

// 击打事件队列
typedef struct {
    uint8_t leaf_id;       // 扇叶ID (1-5)
    uint8_t ring;          // 环数
    uint8_t striking_flag; // 击打标志
} StrikeEvent_t;

#define STRIKE_QUEUE_SIZE 5
static volatile StrikeEvent_t strike_queue[STRIKE_QUEUE_SIZE];
static volatile uint8_t sq_head = 0;
static volatile uint8_t sq_tail = 0;

static uint8_t pending_new_round = 0; // 1 = 上一帧已发OFF，本帧选新扇叶
uint8_t post_hit_window = 0;          // 1 = 处于击中后1s等待窗口（Timer.c读此变量）

/*=== 令牌式反馈去重 ===
 * 每次开始新一轮时 round_id 自增（uint8_t 自然溢出回绕）。
 * 主控在命令帧 byte[4] 中下发当前 round_id；
 * 扇叶在反馈帧 byte[3] 中原样回传。
 * 主控在 RX 回调中丢弃 round_id 不匹配的帧，
 * 从而安全过滤延迟重发和跨轮乱序报文。 */
static uint8_t round_id = 0;

/* 主控端时间门限（ms）—— 安全兜底，防止 EXTI 未关好时的瞬态误报 */
#define MIN_MASTER_ACCEPT_MS  300U
static uint32_t striking_start_tick[5] = {0U};

/*=== 激活进度跟踪 ===
 * 大能量机关共 5 组，每组随机点亮 2 个装甲模块。
 * 击中该组中第一块装甲 = 成功激活该组 → activated_groups++
 * 第二块是否命中不影响该组是否成功。
 * 灯臂中部灯效指示进度：亮起 activated_groups/5。
 * 2.5s 超时（未命中）→ 该组未激活，进度不变，重新选 2 个再试。 */
uint8_t activated_groups = 0;            // 已成功激活的组数 (0-5)，达到5时完全激活
static uint8_t current_group_activated = 0;  // 当前组是否已计入进度（1=已计入，防止第二个扇叶重复计数）

static RGB_t R_logo[64];   //存放R标志数据

uint8_t leaf1 = 0xFF;  // 0xFF = 无效，防止启动时未初始化就有CAN消息匹配到索弓0
                       // 必须等timeout分支设置0xFF后再pending_new_round选择真实扇叶
uint8_t leaf2 = 0xFF;
static uint8_t              LEAF1_data[8];
static uint8_t              LEAF2_data[8];
static uint8_t              LEAF3_data[8];
static uint8_t              LEAF4_data[8];
static uint8_t              LEAF5_data[8];

void LED_Init()
{
    /*Timer_reset();*/
    timeout = 1;  // 初始化时触发超时，让LED_Logic初始化leaf1和leaf2
}


/*CAN中断回调函数（处理扇叶反馈）----------------------------------------------------------------------------*/
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];

    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data);

    // PA7翻转表示收到CAN消息（已经正常工作）
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_7);

    // 去重：丢弃 round_id 不匹配的帧（旧轮延迟重发 / 跨轮乱序）
    if (rx_data[3] != round_id)
        return;

    // 一致性校验：StdId 应与帧内 leaf_id 字段对应（0x010 + leaf_id）
    if (rx_header.StdId != 0x010U + rx_data[0])
        return;

    // 写入队列（环形缓冲区），防止覆盖
    uint8_t next_tail = (sq_tail + 1) % STRIKE_QUEUE_SIZE;
    if (next_tail != sq_head)  // 队列未满才写入
    {
        strike_queue[sq_tail].leaf_id       = rx_data[0];
        strike_queue[sq_tail].striking_flag = rx_data[1];
        strike_queue[sq_tail].ring          = rx_data[2];
        sq_tail = next_tail;
    }

}
/*-------------------------------------------------------------------------------------*/


void LED_Logic(void)
{
    /*==========================================================
     *  大能量机关规则（激活态）：
     *  1) 随机亮 2/5 个装甲模块 → LEAF_STRIKING
     *  2) 2.5s 内命中其中一个 → 该臂灯效变化 (LEAF_STRUCK)
     *     ** 击中第一块 = 成功激活该组，activated_groups++ **
     *  3) 随后 1s 内可以击中另一个被点亮的装甲模块
     *  4) 1s 窗口到期 → 无论第二个是否命中 → 全 OFF → 随机亮新 2 个
     *  5) 2.5s 内无命中 → 该组未激活（进度不变）→ 全 OFF → 重新亮 2 个
     *  6) 灯臂中部指示激活进度：activated_groups / 5
     *  7) 5 组全部激活 → 大符激活完成
     *  每轮从全部 5 个中选（不排除之前用过的）
     *==========================================================*/

    // ====== 2.5s 超时：无命中 → 该组未激活 → 全 OFF → 下一帧重新选 ======
    if (timeout == 1)
    {
        timeout = 0;
        post_hit_window = 0;
        post_hit_timeout = 0;
        sq_head = sq_tail = 0;
        leaf1 = 0xFF;
        leaf2 = 0xFF;
        current_group_activated = 0;   // 该组未命中，不计入进度

        for (uint8_t i = 0; i < 5; i++)
        {
            leafmode[i] = LEAF_OFF;
            leaf_ring_value[i] = 0;
        }
        pending_new_round = 1;
    }
    // ====== 1s 窗口到期 → 无论第二个是否命中 → 全 OFF → 下一帧选新扇叶 ======
    else if (post_hit_timeout == 1)
    {
        post_hit_timeout = 0;
        post_hit_window = 0;
        sq_head = sq_tail = 0;
        leaf1 = 0xFF;
        leaf2 = 0xFF;
        current_group_activated = 0;   // 为下一组做准备

        for (uint8_t i = 0; i < 5; i++)
        {
            leafmode[i] = LEAF_OFF;
            leaf_ring_value[i] = 0;
        }
        pending_new_round = 1;
    }
    // ====== 新一轮：从全部 5 个中随机选 2 个 ======
    else if (pending_new_round == 1)
    {
        pending_new_round = 0;
        round_id++;   // 新一轮令牌：令 RX 自动丢弃上轮遗留的延迟帧

        /* 5 组全部激活 → 已激活状态保持一段时间后自动重置开始新一轮 */
        if (activated_groups >= 5)
        {
            static uint8_t  fully_activated = 0;
            static uint32_t fully_activated_tick = 0;
            #define FULLY_ACTIVATED_HOLD_MS  3000U  // 全亮保持 3 秒后重置

            if (fully_activated == 0)
            {
                /* 首次进入全亮：设置所有灯臂全亮，记录时间 */
                for (uint8_t i = 0; i < 5; i++)
                {
                    leafmode[i] = LEAF_STRUCK;
                    leaf_ring_value[i] = 10;
                }
                leaf1 = 0xFF;
                leaf2 = 0xFF;
                fully_activated = 1;
                fully_activated_tick = HAL_GetTick();
                return;
            }
            else if ((HAL_GetTick() - fully_activated_tick) >= FULLY_ACTIVATED_HOLD_MS)
            {
                /* 保持时间到：重置所有状态，开始新一轮 5 组 */
                fully_activated = 0;
                activated_groups = 0;
                current_group_activated = 0;
                total_struck = 0;
                for (uint8_t i = 0; i < 5; i++)
                {
                    leafmode[i] = LEAF_OFF;
                    leaf_ring_value[i] = 0;
                }
                leaf1 = 0xFF;
                leaf2 = 0xFF;
                sq_head = sq_tail = 0;
                pending_new_round = 1;  // 下一帧选新扇叶
                return;
            }
            else
            {
                /* 保持全亮中，不做其他操作 */
                return;
            }
        }

        /* 随机选 2 个不同的叶片（0-4） */
        leaf1 = rand_create() % 5;
        uint8_t retry = 0;
        do {
            leaf2 = rand_create() % 5;
            retry++;
        } while (leaf2 == leaf1 && retry < 20);
        if (leaf2 == leaf1) leaf2 = (leaf1 + 1) % 5;

        leafmode[leaf1] = LEAF_STRIKING;
        leafmode[leaf2] = LEAF_STRIKING;
        striking_start_tick[leaf1] = HAL_GetTick();
        striking_start_tick[leaf2] = HAL_GetTick();
        sq_head = sq_tail = 0;   // 丢弃旧的队列事件
        Timer_reset();            // 开始 2.5s 倒计时
    }
    // ====== 处理击打事件队列 ======
    else if (sq_head != sq_tail)
    {
        StrikeEvent_t ev = strike_queue[sq_head];
        sq_head = (sq_head + 1) % STRIKE_QUEUE_SIZE;

        /* 有效性校验 */
        if (ev.leaf_id < 1 || ev.leaf_id > 5 || ev.striking_flag != 1)
            return;

        uint8_t idx = ev.leaf_id - 1;  // 1-5 → 0-4

        /* 仅接受当前激活的扇叶 */
        if (idx != leaf1 && idx != leaf2)
            return;

        /* 仅接受处于 STRIKING 状态的扇叶（已被击中变为 OFF 的忽略） */
        if (leafmode[idx] != LEAF_STRIKING)
            return;

        /* 主控端时间门限：过滤激活瞬间的噪声 */
        if ((HAL_GetTick() - striking_start_tick[idx]) < MIN_MASTER_ACCEPT_MS)
            return;

        /* ---- 确认有效命中 ---- */
        total_struck++;
        leafmode[idx] = LEAF_OFF;  // 被击中的扇叶立即关闭（不进入 STRUCK 模式）

        if (post_hit_window == 0)
        {
            /* === 该组首次命中 ===
             * 击中第一块装甲 = 成功激活该组 → 进度 +1
             * 视觉变化：进度条前进一格（所有扇叶同步显示）
             * 另一个扇叶保持 STRIKING，开启 1s 窗口等他击中第二块。 */
            if (current_group_activated == 0 && activated_groups < 5)
            {
                current_group_activated = 1;
                activated_groups++;   // 以组计数：每组只在首次命中时+1
            }
            Timer_reset();
            post_hit_window = 1;
        }
        /* else: 1s 窗口内第二次命中。
         * 该扇叶直接关闭（OFF），不做额外操作。
         * 等 1s 窗口自然到期后进入新一轮。 */
    }
}

void LED_Padding2Update()
{
    // 先执行逻辑处理（包括处理击打反馈）
    LED_Logic();
    
    // 然后准备数据并发送给所有扇叶
    LEAF1_data[0] = LEAF2_data[0] = LEAF3_data[0] = LEAF4_data[0] = LEAF5_data[0] = current_color;
    // data[3] = 激活进度，叶片端灯带中部用此值显示进度条
    LEAF1_data[3] = LEAF2_data[3] = LEAF3_data[3] = LEAF4_data[3] = LEAF5_data[3] = activated_groups;
    // data[4] = round_id 令牌，叶片回传此值，主控据此去重
    LEAF1_data[4] = LEAF2_data[4] = LEAF3_data[4] = LEAF4_data[4] = LEAF5_data[4] = round_id;
    static RGB_t R_color = {255, 0, 0};
    if(current_color == BlueState)
    {
        R_color = R_blue;
    }
    else
    {
        R_color = R_red;
    }
        LEAF1_data[1] = leafmode[LEAF_1];
        LEAF1_data[2] = leaf_ring_value[LEAF_1];
        /*printf("data1:%d\r\n",LEAF1_data[1]);
        printf("data1ring:%d\r\n",LEAF1_data[2]);*/
        //LEAF_0_deliver(LEAF1_data);
        /*while (LEAF_1_deliver(LEAF1_data) != 1);*/
        LEAF_1_deliver(LEAF1_data);


        LEAF2_data[1] = leafmode[LEAF_2];
        LEAF2_data[2] = leaf_ring_value[LEAF_2];
    /*printf("data2:%d\r\n",LEAF2_data[1]);
    printf("data2ring:%d\r\n",LEAF2_data[2]);*/
        //while (LEAF_2_deliver(LEAF2_data) != 1);
    LEAF_2_deliver(LEAF2_data);

        LEAF3_data[1] = leafmode[LEAF_3];
        LEAF3_data[2] = leaf_ring_value[LEAF_3];
    /*printf("data3:%d\r\n",LEAF3_data[1]);
    printf("data3ring:%d\r\n",LEAF3_data[2]);*/
        //while (LEAF_3_deliver(LEAF3_data) != 1);
    LEAF_3_deliver(LEAF3_data);

        LEAF4_data[1] = leafmode[LEAF_4];
        LEAF4_data[2] = leaf_ring_value[LEAF_4];
    /*printf("data4:%d\r\n",LEAF4_data[1]);
    printf("data4ring:%d\r\n",LEAF4_data[2]);*/
        //while (LEAF_4_deliver(LEAF4_data) != 1);
    LEAF_4_deliver(LEAF4_data);

        LEAF5_data[1] = leafmode[LEAF_5];
        LEAF5_data[2] = leaf_ring_value[LEAF_5];
        /*printf("data5:%d\r\n",LEAF5_data[1]);
        printf("data5ring:%d\r\n",LEAF5_data[2]);
        while (LEAF_5_deliver(LEAF5_data) != 1);*/
    LEAF_5_deliver(LEAF5_data);
    //data_send();
    //HAL_Delay(40);

    // HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);

    /*else
    {*/
    //CAN_cmd_chassis_reset_ID();

    while (!ws2812b_IsReady());
    //pack R logo data
    for (uint8_t i = 0; i < 64; i++)
    {
        R_logo[i] = R_color;
    }
    ws2812b_M1H1_SendRGB(R_logo, 64);


}

