#ifndef __SDO_FRAMES_H
#define __SDO_FRAMES_H

#include "sys.h"
#include "can.h"

#define SDO_MAX_DATA_LEN 8
#define ACTION_NODE_COUNT 4

typedef struct {
    UNS8 len;
    UNS8 data[SDO_MAX_DATA_LEN];
} SDO_Frame;

extern const SDO_Frame SDO_ACTIVATE_PPM;
extern const SDO_Frame SDO_DISABLE;
extern const SDO_Frame SDO_ENABLE;
extern const SDO_Frame SDO_GO;
extern const SDO_Frame SDO_TARGET_POS_NODE10000;
extern const SDO_Frame SDO_TARGET_POS_NODE1000;
extern const SDO_Frame SDO_TARGET_POS_NODE2000;
extern const SDO_Frame SDO_TARGET_POS_NODE5000;

UNS8 send_sdo_to_node(UNS8 node_id, const SDO_Frame *frame);
void send_frame_to_action_nodes(const SDO_Frame *frames[ACTION_NODE_COUNT]);
void send_common_frame_to_action_nodes(const SDO_Frame *frame);

#endif
