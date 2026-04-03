#ifndef INC_12_17_LIGHT_H
#define INC_12_17_LIGHT_H

#include "ws2812b_hal.h"

/* 板子类型定义 (编译时配置) */
#define BOARD_TYPE_DEMO   1
#define BOARD_TYPE_TARGET 2
#define BOARD_TYPE        BOARD_TYPE_TARGET // 修改这里来切换板子类型

#define LED_DEMO_NUM (21)
#define LED_TARGET_NUM (271)
#define LED_RECTANGLE_ARROWS_NUM (256)
#define LED_STRIP_NUM (50)
#define LED_FRAMEDATA1_NUM (87)
#define LED_FRAMEDATA2_NUM (93)
typedef enum {
    MODE_DaFu   = 0,  // 环数模式：单叶依次，裁判输入环数
    MODE_XiaoFu = 1   // 双叶激活组模式：每次亮2个，按组计数
} GameMode_t;

typedef enum { LEAF_OFF, LEAF_STRIKING, LEAF_STRUCK} LED_Leaf_Mode_t;
typedef enum { RedState, BlueState, DebugState} LED_State_t;

/* 一键切换控制源：本地按键直控 or 主控CAN下发 */
#define LEAF_CONTROL_LOCAL 0U
#define LEAF_CONTROL_CAN   1U
#define LEAF_CONTROL_MODE  LEAF_CONTROL_lOCAL

/* 本地直控模式下，命中后是否锁存显示（1=锁存，0=松手即灭） */
#define LOCAL_HIT_LATCH_ENABLE 0U

/* target 单独全亮测试：1=只输出 PA8 target 全白，0=正常逻辑 */
#define TARGET_FULL_ON_TEST_MODE 0U

static const RGB_t red = {185, 0, 0};
static const RGB_t blue = {0, 0, 125};
static const RGB_t off = {0, 0, 0};

extern LED_Leaf_Mode_t leafmode; // 当前叶片状态，供外部模块读取
extern LED_State_t ledstate; // 当前LED状态，供外部模块读取
extern GameMode_t gamemode; // 当前游戏模式，供外部模块读取
extern uint8_t received_round_id;
void Light_Init(LED_State_t state); // LED初始化函数声明
void led_debug(LED_Leaf_Mode_t leafmode, RGB_t *dst); // LED调试函数声明
void demoData(LED_Leaf_Mode_t leafmode, RGB_t *dst); // LED演示数据函数声明
void TargetData(LED_Leaf_Mode_t leafmode, RGB_t *dst); // 目标数据函数声明
void RectangleArrowsData(LED_Leaf_Mode_t leafmode, RGB_t *dst); // 矩形箭头数据函数声明
void StripData(LED_Leaf_Mode_t leafmode, RGB_t *dst); // 条形数据函数声明
void LED_PackFrameData1(LED_Leaf_Mode_t leafmode, RGB_t *dst); // 前框架数据函数声明
void LED_PackFrameData2(LED_Leaf_Mode_t leafmode, RGB_t *dst); // 后框架数据函数声明
void Target_FullOn_Test(void); // target 全亮测试函数声明
void led_update(void); // LED更新函数声明   
#endif //INC_12_17_LIGHT_H