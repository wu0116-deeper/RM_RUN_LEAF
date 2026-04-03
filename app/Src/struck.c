#include "struck.h"
#include "gpio.h"
#include "light.h"
#include  "deliver.h"

#ifndef LOCAL_KEY_ACTIVE_LEVEL
#define LOCAL_KEY_ACTIVE_LEVEL GPIO_PIN_RESET
#endif

uint8_t struck_ring = 0;           // 当前有效的环数（用于显示）
volatile uint8_t struck_flag = 0;
static uint8_t last_struck_ring = 0;  // 上一次检测到的击打环数
static uint8_t new_hit_flag = 0;      // 新击打标志（用于区分新击打和保持状态）

void ReadStruck(void) {
    GPIO_TypeDef* Ports[] = {KEY1_GPIO_Port, KEY2_GPIO_Port, KEY3_GPIO_Port, KEY4_GPIO_Port,KEY5_GPIO_Port,KEY6_GPIO_Port,KEY7_GPIO_Port, KEY9_GPIO_Port, KEY8_GPIO_Port};
    uint16_t Pins[] = {KEY1_Pin, KEY2_Pin, KEY3_Pin, KEY4_Pin, KEY5_Pin, KEY6_Pin, KEY7_Pin, KEY9_Pin, KEY8_Pin};

    uint8_t current_hit_ring = 0;  // 当前帧检测到的环数（0表示无击打）

    for (int i = 0; i < 9; i++) {
        if (HAL_GPIO_ReadPin(Ports[i], Pins[i]) == LOCAL_KEY_ACTIVE_LEVEL) {
            current_hit_ring = i + 1;
            break;  // 检测到一个就退出
        }
    }

    if (current_hit_ring != 0) {
        // 有击打
        if (current_hit_ring != last_struck_ring) {
            // 新的环数被击打 → 清零旧状态，设置新环数
            struck_ring = current_hit_ring;
            new_hit_flag = 1;
        }
        // else: 同一环数持续按住 → 保持当前状态，不做处理

        last_struck_ring = current_hit_ring;
        struck_flag = 1;
    } else {
        // 没有击打 → 保持当前 struck_ring 不变
        last_struck_ring = 0;  // 重置，这样下次任意环数击打都算"新"
        struck_flag = 0;
    }
}

// 反馈数据函数：将击打数据发送给主控
void send_strike_data(void)
{
    if(struck_flag == 1)
    {
        uint8_t control_data[8] = {0};
        control_data[0] = LEAF_BOARD_ID;
        control_data[1] = struck_flag;
        control_data[2] = struck_ring;
        control_data[3] = received_round_id;  // 回传本轮令牌，主控靠此去重

        control_deliver(control_data);
        struck_flag = 0;
    }
}

