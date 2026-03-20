#include "stm32f4xx_it.h"
#include "main.h"
#include <string.h>

#define ACTION_NODE_COUNT     4
#define SDO_MAX_DATA_LEN      8
#define ACTION_FRAME_DELAY_MS 5
#define ACTION_DEBOUNCE_MS    20

typedef struct {
    UNS8 len;
    UNS8 data[SDO_MAX_DATA_LEN];
} SDO_Frame;

static const SDO_Frame SDO_ACTIVATE_PPM = {5, {0x2F, 0x60, 0x60, 0x00, 0x01}};
static const SDO_Frame SDO_DISABLE = {6, {0x2B, 0x40, 0x60, 0x00, 0x06, 0x00}};
static const SDO_Frame SDO_ENABLE = {6, {0x2B, 0x40, 0x60, 0x00, 0x0F, 0x00}};
static const SDO_Frame SDO_GO = {6, {0x2B, 0x40, 0x60, 0x00, 0x5F, 0x00}};

static const SDO_Frame SDO_TARGET_POS_10000 = {8, {0x23, 0x7A, 0x60, 0x00, 0x10, 0x27, 0x00, 0x00}};
static const SDO_Frame SDO_TARGET_POS_1000 = {8, {0x23, 0x7A, 0x60, 0x00, 0xE8, 0x03, 0x00, 0x00}};
static const SDO_Frame SDO_TARGET_POS_2000 = {8, {0x23, 0x7A, 0x60, 0x00, 0xD0, 0x07, 0x00, 0x00}};
static const SDO_Frame SDO_TARGET_POS_5000 = {8, {0x23, 0x7A, 0x60, 0x00, 0x88, 0x13, 0x00, 0x00}};

static volatile uint8_t action_is_busy = 0;

static UNS8 send_sdo_to_node(UNS8 node_id, const SDO_Frame *frame)
{
    Message tx = {0};

    if ((frame == 0) || (frame->len > SDO_MAX_DATA_LEN)) {
        return 0;
    }

    tx.cob_id = 0x600 + node_id;
    tx.len = frame->len;
    tx.rtr = 0;
    memcpy(tx.data, frame->data, frame->len);

    return canSend(CAN1, &tx);
}

static void send_common_frame_to_action_nodes(const SDO_Frame *frame)
{
    UNS8 node_id;

    if (frame == 0) {
        return;
    }

    for (node_id = 1; node_id <= ACTION_NODE_COUNT; ++node_id) {
        send_sdo_to_node(node_id, frame);
    }
}

static void send_frame_to_action_nodes(const SDO_Frame *frames[ACTION_NODE_COUNT])
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

static void key_exti_init(void)
{
    EXTI_InitTypeDef exti_init_structure;
    NVIC_InitTypeDef nvic_init_structure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource1);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource13);

    EXTI_ClearITPendingBit(EXTI_Line1 | EXTI_Line13);

    exti_init_structure.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_init_structure.EXTI_Trigger = EXTI_Trigger_Falling;
    exti_init_structure.EXTI_LineCmd = ENABLE;

    exti_init_structure.EXTI_Line = EXTI_Line1;
    EXTI_Init(&exti_init_structure);

    exti_init_structure.EXTI_Line = EXTI_Line13;
    EXTI_Init(&exti_init_structure);

    nvic_init_structure.NVIC_IRQChannel = EXTI1_IRQn;
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_init_structure.NVIC_IRQChannelSubPriority = 0;
    nvic_init_structure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init_structure);

    nvic_init_structure.NVIC_IRQChannel = EXTI15_10_IRQn;
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_init_structure.NVIC_IRQChannelSubPriority = 1;
    nvic_init_structure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init_structure);
}

static void run_action_common_prefix(void)
{
    send_common_frame_to_action_nodes(&SDO_ACTIVATE_PPM);
    delay_ms(ACTION_FRAME_DELAY_MS);

    send_common_frame_to_action_nodes(&SDO_DISABLE);
    delay_ms(ACTION_FRAME_DELAY_MS);

    send_common_frame_to_action_nodes(&SDO_ENABLE);
    delay_ms(ACTION_FRAME_DELAY_MS);
}

static void run_action_one(void)
{
    const SDO_Frame *target_frames[ACTION_NODE_COUNT] = {
        &SDO_TARGET_POS_10000,
        &SDO_TARGET_POS_10000,
        &SDO_TARGET_POS_10000,
        &SDO_TARGET_POS_10000
    };

    PDout(2) = 0;
    PAout(8) = 1;

    run_action_common_prefix();
    send_frame_to_action_nodes(target_frames);
    delay_ms(ACTION_FRAME_DELAY_MS);
    send_common_frame_to_action_nodes(&SDO_GO);

    PDout(2) = 1;
}

static void run_action_two(void)
{
    const SDO_Frame *target_frames[ACTION_NODE_COUNT] = {
        &SDO_TARGET_POS_1000,
        &SDO_TARGET_POS_2000,
        &SDO_TARGET_POS_5000,
        &SDO_TARGET_POS_10000
    };

    PAout(8) = 0;
    PDout(2) = 1;

    run_action_common_prefix();
    send_frame_to_action_nodes(target_frames);
    delay_ms(ACTION_FRAME_DELAY_MS);
    send_common_frame_to_action_nodes(&SDO_GO);

    PAout(8) = 1;
}

static void try_run_action(uint32_t exti_line)
{
    if (action_is_busy != 0) {
        return;
    }

    action_is_busy = 1;
    delay_ms(ACTION_DEBOUNCE_MS);

    if ((exti_line == EXTI_Line1) && (GPIO_ReadInputDataBit(KEY0_GPIO_PORT, KEY0_GPIO_PIN) == Bit_RESET)) {
        run_action_one();
    }
    else if ((exti_line == EXTI_Line13) && (GPIO_ReadInputDataBit(KEY1_GPIO_PORT, KEY1_GPIO_PIN) == Bit_RESET)) {
        run_action_two();
    }

    action_is_busy = 0;
}

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
    while (1)
    {
    }
}

void MemManage_Handler(void)
{
    while (1)
    {
    }
}

void BusFault_Handler(void)
{
    while (1)
    {
    }
}

void UsageFault_Handler(void)
{
    while (1)
    {
    }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
}

void EXTI1_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line1) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line1);
        try_run_action(EXTI_Line1);
    }
}

void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line13) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line13);
        try_run_action(EXTI_Line13);
    }
}

void board_irq_init(void)
{
    key_exti_init();
}
