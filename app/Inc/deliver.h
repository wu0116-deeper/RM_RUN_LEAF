#ifndef DELIVER_H
#define DELIVER_H

void can_filter_init(void);
extern uint8_t control_deliver(uint8_t *send_data);
extern uint32_t LEAF_BOARD_ID; // 扇叶板ID（声明）

#endif
