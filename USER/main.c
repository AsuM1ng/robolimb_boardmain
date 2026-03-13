#include "sys.h"
#include "main.h"
#include "sdo_frames.h"

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    delay_init(168);

    CAN1_Init(&Master_Data,1000000);
    TIM2_Init();
    USART1_Init(115200);

    setNodeId(&Master_Data, 0x01);
    setState(&Master_Data, Initialisation);
    setState(&Master_Data, Operational);

    while(1)
    {
        /*
         * 依次发往 0x601~0x604（不同节点不同报文）。
         * 0x01：肩水平
         */
        send_sdo_to_node(0x01, &SDO_ACTIVATE_PPM);
        send_sdo_to_node(0x02, &SDO_ACTIVATE_PPM);
        send_sdo_to_node(0x03, &SDO_ACTIVATE_PPM);
        send_sdo_to_node(0x04, &SDO_ACTIVATE_PPM);
        delay_ms(5);

        send_sdo_to_node(0x01, &SDO_DISABLE);
        send_sdo_to_node(0x02, &SDO_DISABLE);
        send_sdo_to_node(0x03, &SDO_DISABLE);
        send_sdo_to_node(0x04, &SDO_DISABLE);
        delay_ms(5);

        send_sdo_to_node(0x01, &SDO_ENABLE);
        send_sdo_to_node(0x02, &SDO_ENABLE);
        send_sdo_to_node(0x03, &SDO_ENABLE);
        send_sdo_to_node(0x04, &SDO_ENABLE);
        delay_ms(5);

        send_sdo_to_node(0x01, &SDO_TARGET_POS_NODE10000);
        send_sdo_to_node(0x02, &SDO_TARGET_POS_NODE10000);
        send_sdo_to_node(0x03, &SDO_TARGET_POS_NODE10000);
        send_sdo_to_node(0x04, &SDO_TARGET_POS_NODE10000);
        delay_ms(5);

        send_sdo_to_node(0x01, &SDO_GO);
        send_sdo_to_node(0x02, &SDO_GO);
        send_sdo_to_node(0x03, &SDO_GO);
        send_sdo_to_node(0x04, &SDO_GO);

        delay_ms(1000);
    }
}
