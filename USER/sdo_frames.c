#include "sdo_frames.h"
#include "can_driver.h"
#include <string.h>

const SDO_Frame SDO_ACTIVATE_PPM      = {5, {0x2F,0x60,0x60,0x00,0x01}};
const SDO_Frame SDO_DISABLE           = {6, {0x2B,0x40,0x60,0x00,0x06,0x00}};
const SDO_Frame SDO_ENABLE            = {6, {0x2B,0x40,0x60,0x00,0x0F,0x00}};
const SDO_Frame SDO_GO                = {6, {0x2B,0x40,0x60,0x00,0x5F,0x00}};

const SDO_Frame SDO_TARGET_POS_NODE10000  = {8, {0x23,0x7A,0x60,0x00,0x10,0x27,0x00,0x00}};
const SDO_Frame SDO_TARGET_POS_NODE1000   = {8, {0x23,0x7A,0x60,0x00,0xE8,0x03,0x00,0x00}};
const SDO_Frame SDO_TARGET_POS_NODE2000   = {8, {0x23,0x7A,0x60,0x00,0xD0,0x07,0x00,0x00}};
const SDO_Frame SDO_TARGET_POS_NODE5000   = {8, {0x23,0x7A,0x60,0x00,0x88,0x13,0x00,0x00}};

UNS8 send_sdo_to_node(UNS8 node_id, const SDO_Frame *frame)
{
    Message tx = {0};

    if (frame == 0 || frame->len > SDO_MAX_DATA_LEN) {
        return 0;
    }

    tx.cob_id = 0x600 + node_id;
    tx.len = frame->len;
    tx.rtr = 0;
    memcpy(tx.data, frame->data, frame->len);

    return canSend(0, &tx);
}

void send_frame_to_action_nodes(const SDO_Frame *frames[ACTION_NODE_COUNT])
{
    UNS8 node_id;

    if (frames == 0) {
        return;
    }

    for (node_id = 0; node_id < ACTION_NODE_COUNT; ++node_id) {
        if (frames[node_id] != 0) {
            send_sdo_to_node(node_id + 1, frames[node_id]);
        }
    }
}

void send_common_frame_to_action_nodes(const SDO_Frame *frame)
{
    UNS8 node_id;

    if (frame == 0) {
        return;
    }

    for (node_id = 1; node_id <= ACTION_NODE_COUNT; ++node_id) {
        send_sdo_to_node(node_id, frame);
    }
}
