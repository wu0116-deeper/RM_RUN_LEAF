#include "stm32f1xx_hal.h"
#include "deliver.h"
#include "can.h"


uint32_t LEAF_BOARD_ID = 0x05; // 扇叶板ID（定义）
static CAN_TxHeaderTypeDef control_Txmsg;

void can_filter_init(void)
{

    CAN_FilterTypeDef can_filter_st;
    /*uint32_t std_id = 0x700;*/
    uint32_t std_id = LEAF_BOARD_ID; //接收端地址，五块扇叶板地址需要不一样且与主控代码规定的一致

    can_filter_st.FilterActivation = ENABLE;
    can_filter_st.FilterMode = CAN_FILTERMODE_IDMASK;
    /*can_filter_st.FilterMode = CAN_FILTERMODE_IDLIST;*/
    can_filter_st.FilterScale = CAN_FILTERSCALE_32BIT;
    can_filter_st.FilterIdHigh = std_id<<5;
    can_filter_st.FilterIdLow = 0x0000 | CAN_ID_STD;
    can_filter_st.FilterMaskIdHigh = 0xFFE0;
    can_filter_st.FilterMaskIdLow = 0x0000;
    can_filter_st.FilterBank = 0;
    can_filter_st.FilterFIFOAssignment = CAN_RX_FIFO0;
    HAL_CAN_ConfigFilter(&hcan, &can_filter_st);
    HAL_CAN_Start(&hcan);
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

}

/**
  * @brief  向主控CAN总线发送击打结果数据
  * @param  send_data: 8字节数据数组指针
  *         [0] = LEAF_BOARD_ID (扇叶板ID: 1-5)
  *         [1] = striking_flag (击打标志: 1=击中)
  *         [2] = current_ring  (环数信息)
  *         [3] = received_round_id (本轮令牌，由主控下发，回传用于去重)
  *         [4-7] = 保留字段
  * @retval 0 (当前实现总是返回0)
  * @note   发送地址为 0x010+LEAF_BOARD_ID（0x011-0x015），主控用 mask 0x7F0 统一接收该段
  */
uint8_t control_deliver(uint8_t *send_data)
{
    uint32_t send_mail_box;

    control_Txmsg.StdId = 0x010U + LEAF_BOARD_ID; //反馈地址：0x011-0x015（主控用 mask 0x7F0 接收 0x010-0x01F 段）
    control_Txmsg.IDE = CAN_ID_STD;    // 标准帧
    control_Txmsg.RTR = CAN_RTR_DATA;  // 数据帧
    control_Txmsg.DLC = 0x08;          // 数据长度8字节

    // 通过CAN总线发送数据到主控
    HAL_CAN_AddTxMessage(&hcan, &control_Txmsg, send_data, &send_mail_box);
    HAL_Delay(10);  // 等待发送完成

    return 0;
}