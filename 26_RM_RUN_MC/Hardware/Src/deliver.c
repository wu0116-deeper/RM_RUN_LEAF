//
// Created by Lenovo on 2024/1/22.
//

#include "stm32f1xx_hal.h"
#include "deliver.h"
#include "can.h"

static CAN_TxHeaderTypeDef  chassis_tx_message;

static CAN_TxHeaderTypeDef  LEAF_0_Txmsg;
static CAN_TxHeaderTypeDef  LEAF_1_Txmsg;
static CAN_TxHeaderTypeDef  LEAF_2_Txmsg;
static CAN_TxHeaderTypeDef  LEAF_3_Txmsg;
static CAN_TxHeaderTypeDef  LEAF_4_Txmsg;
static CAN_TxHeaderTypeDef  LEAF_5_Txmsg;
/*static CAN_TxHeaderTypeDef  LEAF_1_Txmsg;*/

static uint8_t              chassis_can_send_data[8];
/*void CAN_cmd_chassis_reset_ID(void)
{
    uint32_t send_mail_box;
    chassis_tx_message.StdId = 0x700;
    chassis_tx_message.IDE = CAN_ID_STD;
    chassis_tx_message.RTR = CAN_RTR_DATA;
    chassis_tx_message.DLC = 0x08;
    chassis_can_send_data[0] = 0;
    chassis_can_send_data[1] = 0;
    chassis_can_send_data[2] = 0;
    chassis_can_send_data[3] = 0;
    chassis_can_send_data[4] = 0;
    chassis_can_send_data[5] = 0;
    chassis_can_send_data[6] = 0;
    chassis_can_send_data[7] = 0;

    HAL_CAN_AddTxMessage(&hcan, &chassis_tx_message, chassis_can_send_data, &send_mail_box);
}*/

uint8_t LEAF_0_deliver(uint8_t *send_data)
{
    uint32_t send_mail_box;
    LEAF_0_Txmsg.StdId = 0x600;
    LEAF_0_Txmsg.IDE = CAN_ID_STD;
    LEAF_0_Txmsg.RTR = CAN_RTR_DATA;
    LEAF_0_Txmsg.DLC = 0x08;
    //while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan)==0);
    /*if (HAL_CAN_AddTxMessage(&hcan, &LEAF_1_Txmsg, send_data, &send_mail_box) != HAL_OK)
    return 0;
    else
        return 1;*/
    //while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan)==0);
    HAL_CAN_AddTxMessage(&hcan, &LEAF_0_Txmsg, send_data, &send_mail_box) ;
    HAL_Delay(10);
    /*while(HAL_CAN_AddTxMessage(&hcan, &LEAF_1_Txmsg, send_data, &send_mail_box) != HAL_OK);*/
}

/*
 * 以下五个函数为向五块扇叶板发送数据的函数，务必保证五块扇叶板的接收地址与这五个函数中规定的地址一致，顺序先后没有影响
 */
uint8_t LEAF_1_deliver(uint8_t *send_data)
{
    uint32_t send_mail_box;
    LEAF_1_Txmsg.StdId = 0x01; //规定地址的代码，后面四个函数是一样的
    LEAF_1_Txmsg.IDE = CAN_ID_STD;
    LEAF_1_Txmsg.RTR = CAN_RTR_DATA;
    LEAF_1_Txmsg.DLC = 0x08;
    //while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan)==0);
    /*if (HAL_CAN_AddTxMessage(&hcan, &LEAF_1_Txmsg, send_data, &send_mail_box) != HAL_OK)
    return 0;
    else
        return 1;*/
    //while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan)==0);
    HAL_CAN_AddTxMessage(&hcan, &LEAF_1_Txmsg, send_data, &send_mail_box) ;
    HAL_Delay(10);
    /*while(HAL_CAN_AddTxMessage(&hcan, &LEAF_1_Txmsg, send_data, &send_mail_box) != HAL_OK);*/
}


uint8_t LEAF_2_deliver(uint8_t *send_data)
{
    uint32_t send_mail_box;
    //LEAF_2_Txmsg.StdId = 0x704;
    LEAF_2_Txmsg.StdId = 0x02;
    LEAF_2_Txmsg.IDE = CAN_ID_STD;
    LEAF_2_Txmsg.RTR = CAN_RTR_DATA;
    LEAF_2_Txmsg.DLC = 0x08;

    /*if (HAL_CAN_AddTxMessage(&hcan, &LEAF_2_Txmsg, send_data, &send_mail_box) != HAL_OK)
        return 0;
    else
        return 1;*/
    HAL_CAN_AddTxMessage(&hcan, &LEAF_2_Txmsg, send_data, &send_mail_box) ;
    HAL_Delay(10);
    /*while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan)==0);
    HAL_CAN_AddTxMessage(&hcan, &LEAF_2_Txmsg, send_data, &send_mail_box) ;*/
}

uint8_t LEAF_3_deliver(uint8_t *send_data)
{
    uint32_t send_mail_box;
    //LEAF_3_Txmsg.StdId = 0x708;
    LEAF_3_Txmsg.StdId = 0x03;
    LEAF_3_Txmsg.IDE = CAN_ID_STD;
    LEAF_3_Txmsg.RTR = CAN_RTR_DATA;
    LEAF_3_Txmsg.DLC = 0x08;

    /*if (HAL_CAN_AddTxMessage(&hcan, &LEAF_3_Txmsg, send_data, &send_mail_box3) != HAL_OK)
        return 0;
    else
        return 1;*/
    HAL_CAN_AddTxMessage(&hcan, &LEAF_3_Txmsg, send_data, &send_mail_box) ;
    HAL_Delay(10);
}

uint8_t LEAF_4_deliver(uint8_t *send_data)
{
    uint32_t send_mail_box;
    //LEAF_4_Txmsg.StdId = 0x712;
    LEAF_4_Txmsg.StdId = 0x04;
    LEAF_4_Txmsg.IDE = CAN_ID_STD;
    LEAF_4_Txmsg.RTR = CAN_RTR_DATA;
    LEAF_4_Txmsg.DLC = 0x08;

    HAL_CAN_AddTxMessage(&hcan, &LEAF_4_Txmsg, send_data, &send_mail_box) ;
    HAL_Delay(10);
}

uint8_t LEAF_5_deliver(uint8_t *send_data)
{
    uint32_t send_mail_box;
    //LEAF_5_Txmsg.StdId = 0x716;
    LEAF_5_Txmsg.StdId = 0x05;
    LEAF_5_Txmsg.IDE = CAN_ID_STD;
    LEAF_5_Txmsg.RTR = CAN_RTR_DATA;
    LEAF_5_Txmsg.DLC = 0x08;

    HAL_CAN_AddTxMessage(&hcan, &LEAF_5_Txmsg, send_data, &send_mail_box) ;
    HAL_Delay(10);
}



/*
void can_filter_init(void)
{

    CAN_FilterTypeDef can_filter_st;
    uint32_t std_id = 0x75F;
    can_filter_st.FilterActivation = ENABLE;
    can_filter_st.FilterMode = CAN_FILTERMODE_IDMASK;
    can_filter_st.FilterScale = CAN_FILTERSCALE_32BIT;
    can_filter_st.FilterIdHigh = std_id << 5;      // 0x006 << 5 = 0x00C0
    can_filter_st.FilterIdLow = 0x0000 | CAN_ID_STD;
    can_filter_st.FilterMaskIdHigh = 0xFFE0;       // 严格匹配全部11位，只接收0x006
    can_filter_st.FilterMaskIdLow = 0x0000;
    can_filter_st.FilterBank = 0;
    can_filter_st.FilterFIFOAssignment = CAN_RX_FIFO0;
    HAL_CAN_ConfigFilter(&hcan, &can_filter_st);
    HAL_CAN_Start(&hcan);
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

}
*/

void can_filter_init(void)
{
    /*
     * 接收扇叶反馈帧段：0x011–0x015（= 0x010 + LEAF_BOARD_ID）
     *
     * Mask 说明（11-bit StdId）：
     *   0x010 = 0b00000010000  bits[10:4] = 0000001, bits[3:0] = 0000
     *   0x01F = 0b00000011111  bits[10:4] = 0000001, bits[3:0] = 1111
     *   ────────────────────
     *   care bits  = [10:4] → mask = 0x7F0 (0b11111110000)
     *   don't-care = [3:0]
     *
     * HAL 32-bit filter：StdId 占 FilterIdHigh[15:5]，即 std_id << 5。
     */
    CAN_FilterTypeDef can_filter_st;

    can_filter_st.FilterActivation     = ENABLE;
    can_filter_st.FilterMode           = CAN_FILTERMODE_IDMASK;
    can_filter_st.FilterScale          = CAN_FILTERSCALE_32BIT;
    can_filter_st.FilterIdHigh         = 0x010U << 5;   // = 0x0200：匹配基地址 0x010
    can_filter_st.FilterIdLow          = 0x0000;
    can_filter_st.FilterMaskIdHigh     = 0x7F0U << 5;   // = 0xFE00：检查 bits[10:4]，放行 bits[3:0]
    can_filter_st.FilterMaskIdLow      = 0x0000;
    can_filter_st.FilterBank           = 0;
    can_filter_st.FilterFIFOAssignment = CAN_RX_FIFO0;
    HAL_CAN_ConfigFilter(&hcan, &can_filter_st);
    HAL_CAN_Start(&hcan);
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}
