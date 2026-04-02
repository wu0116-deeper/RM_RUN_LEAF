//
// Created by Lenovo on 2024/1/22.
//

#ifndef MASTERCONTROL_DELIVER_H
#define MASTERCONTROL_DELIVER_H

void CAN_cmd_chassis_reset_ID(void);
void can_filter_init(void);

uint8_t LEAF_0_deliver(uint8_t *send_data);
uint8_t LEAF_1_deliver(uint8_t *send_data);
uint8_t LEAF_2_deliver(uint8_t *send_data);
uint8_t LEAF_3_deliver(uint8_t *send_data);
uint8_t LEAF_4_deliver(uint8_t *send_data);
uint8_t LEAF_5_deliver(uint8_t *send_data);

#endif //MASTERCONTROL_DELIVER_H
