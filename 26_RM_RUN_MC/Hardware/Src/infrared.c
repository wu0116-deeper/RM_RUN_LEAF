// //
// // Created by Hanjiang
// //
// #include "stm32f1xx_hal.h"
//
// #include "infrared.h"
// #include "Timer.h"
// #include "usart.h"
// #include "ws2812b_hal.h"
// #include "light.h"
//
// uint16_t IR_Time; // 存储每两次下降沿之间的时间
// uint8_t IR_State; // 状态机变量： = 0，等待开始/重复； = 1，开始； = 2
//
// uint8_t IR_Data[4]; // 存储接收到的4字节数据，每字节为8位
// uint8_t IR_pData;   // 指针，指示当前接收第几位数据，总为32
//
// uint8_t IR_DataFlag;    // 数据接收标志，数据接收完成置1
// uint8_t IR_RepeatFlag;  // 重复标志，当接收到重复帧时置1
// uint8_t IR_Address;     // 用于存储地址码
// uint8_t IR_Command;     // 用于存储命令码（按键）
// uint8_t IR_AntiAddress; // 用于存储地址码反码
// uint8_t IR_AntiCommand; // 用于存储命令码反码（按键）
//
// extern LED_State_t current_color;
// extern Leaf_NUM_t current_leaf;//当前叶片
// /*extern RGB_t current_color;
// extern RGB_t current_color_dim;*/
//
// extern uint8_t currentLeafStruck;
// /*extern uint8_t leaf_ring_value[5];*/
// extern uint8_t current_ring;
// extern uint8_t debug;
//
// void LED_set(void);
//
// uint8_t IR_GetDataFlag(void)  // 获取数据接收标志
// {
//     if (IR_DataFlag)
//     {
//         IR_DataFlag = 0;
//         return 1;
//     }
//     else
//     {
//         return 0;
//     }
// }
//
// uint8_t IR_GetRepeatFlag(void) // 获取红外重复标志
// {
//     if (IR_RepeatFlag)
//     {
//         IR_RepeatFlag = 0;
//         return 1;
//     }
//     else
//     {
//         return 0;
//     }
// }
//
// uint8_t IR_GetAddress(void)
// {
//     return IR_Address;
// }
//
// uint8_t IR_GetCommand(void)
// {
//     return IR_Command;
// }
//
// uint8_t IR_GetAntiAddress(void)
// {
//     return IR_AntiAddress;
// }
//
// uint8_t IR_GetAntiCommand(void)
// {
//     return IR_AntiCommand;
// }
//
//
// //Counter
//
//
// //void EXTI3_IRQHandler(void)
//
// void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
// {
//     if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET)
//     {
//         if (IR_State == 0)
//         {
//             TIM4_StartCounter();
//             IR_State = 1;
//         }
//         else if (IR_State == 1)
//         {
//             IR_Time = TIM4_GetCounter();
//             TIM4_StartCounter();
//             /*printf("A\r\n");*/
//             if (IR_Time > 13500 - 500 && IR_Time < 13500 + 500)
//             {
//                 IR_State = 2;
//             }
//             else if (IR_Time > 11250 - 500 && IR_Time < 11250 + 500)
//             {
//                 IR_RepeatFlag = 1;
//                 IR_State = 0;
//             }
//             else
//             {
//                 IR_State = 1;
//             }
//         }
//         else if (IR_State == 2) // 处理数据
//         {
//             IR_Time = TIM4_GetCounter();
//             TIM4_StartCounter();
//
//             if (IR_Time > 1120 - 500 && IR_Time < 1120 + 300) // 逻辑0
//             {
//                 IR_Data[IR_pData / 8] &= ~(0x01 << (IR_pData % 8));
//                 IR_pData++;
//             }
//             else if (IR_Time > 2250 - 500 && IR_Time < 2250 + 500) // 逻辑1
//             {
//                 IR_Data[IR_pData / 8] |= (0x01 << (IR_pData % 8));
//                 IR_pData++;
//             }
//             else // 既不是逻辑0也不是逻辑1 出现错误，重置状态
//             {
//                 IR_pData = 0; // 清空IR_pData
//                 IR_State = 1; // 重新 接收该帧数据，寻找下一帧开始码
//                 /*printf("D\r\n");*/
//             }
//             if (IR_pData >= 32)
//             {
//
//                 IR_pData = 0;
//                 //					if((IR_Data[0] == ~IR_Data[1]) && (IR_Data[2] == ~IR_Data[3]))
//                 //					{	//������һֱ��ͨ������������֮���ַ��붼����
//                 IR_Address = IR_Data[0];
//                 IR_AntiAddress = IR_Data[1];
//                 IR_Command = IR_Data[2];
//                 IR_AntiCommand = IR_Data[3];
//                 IR_DataFlag = 1;    // ���ս���
//
//                 //					}
//                 IR_State = 0;
//                 /*printf("value:%hu\r\n",IR_GetCommand());*/
//                 if(IR_Address == 0x00 )     //防止一直全局变量错乱，还能检测接收效果
//                 {
//                     LED_set();  //根据接收到的红外信号，改变各个相关的状态
//                 }
//             }
//         }
//     }
// }
//
// void LED_set(void)
// {
//     switch(IR_Command)
//     {
//         /*case 0x15:  //确认键 +
//             currentLeafStruck = 1;
//             leaf_ring_value = 0;
//             break;
//         case 0x16:  //上键 /0
//             currentLeafStruck = 1;
//             leaf_ring_value = 1;*/
//             break;
//         case 0x0C:  //1
//             currentLeafStruck = 1;
//             current_ring = 2;
//             break;
//         case 0x18:  //2
//             currentLeafStruck = 1;
//             current_ring = 4;
//             break;
//         case 0x5E:  //3
//             currentLeafStruck = 1;
//             current_ring = 6;
//             break;
//         case 0x08:  //4
//             currentLeafStruck = 1;
//             current_ring = 8;
//             break;
//         case 0x1C:  //5
//             currentLeafStruck = 1;
//             current_ring = 10;
//             break;
//         case 0x16:  //0
//             if(current_color == RedState)
//             {
//                 current_color = BlueState;
//             }
//             else
//             {
//                 current_color = RedState;
//             }
//             break;
//         case 0x15: //+
//             debug = (debug == 0 ? 1 : 0);
//             break;
//     }
// }
//
