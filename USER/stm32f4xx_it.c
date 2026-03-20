/**
  ******************************************************************************
  * @file    Project/STM32F4xx_StdPeriph_Templates/stm32f4xx_it.c
  * @brief   Main Interrupt Service Routines.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "main.h"
#include "sdo_frames.h"

#define LED_ACTION1_PORT GPIOD
#define LED_ACTION1_PIN  GPIO_Pin_2
#define LED_ACTION2_PORT GPIOA
#define LED_ACTION2_PIN  GPIO_Pin_8
#define KEY_ACTION1_PORT GPIOC
#define KEY_ACTION1_PIN  GPIO_Pin_1
#define KEY_ACTION2_PORT GPIOC
#define KEY_ACTION2_PIN  GPIO_Pin_13
#define ACTION_FRAME_DELAY_MS 5
#define ACTION_DEBOUNCE_MS 20

static volatile uint8_t action_is_busy = 0;

static void led_gpio_init(void)
{
    GPIO_InitTypeDef gpio_init_structure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOD, ENABLE);

    gpio_init_structure.GPIO_Mode = GPIO_Mode_OUT;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_PuPd = GPIO_PuPd_UP;
    gpio_init_structure.GPIO_Speed = GPIO_High_Speed;

    gpio_init_structure.GPIO_Pin = LED_ACTION2_PIN;
    GPIO_Init(LED_ACTION2_PORT, &gpio_init_structure);

    gpio_init_structure.GPIO_Pin = LED_ACTION1_PIN;
    GPIO_Init(LED_ACTION1_PORT, &gpio_init_structure);

    GPIO_SetBits(LED_ACTION1_PORT, LED_ACTION1_PIN);
    GPIO_SetBits(LED_ACTION2_PORT, LED_ACTION2_PIN);
}

static void key_exti_init(void)
{
    GPIO_InitTypeDef gpio_init_structure;
    EXTI_InitTypeDef exti_init_structure;
    NVIC_InitTypeDef nvic_init_structure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    gpio_init_structure.GPIO_Mode = GPIO_Mode_IN;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_PuPd = GPIO_PuPd_UP;
    gpio_init_structure.GPIO_Speed = GPIO_High_Speed;
    gpio_init_structure.GPIO_Pin = KEY_ACTION1_PIN | KEY_ACTION2_PIN;
    GPIO_Init(GPIOC, &gpio_init_structure);

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

static void canopen_stack_init(void)
{
    CAN1_Init(&Master_Data, 1000000);
    TIM2_Init();
    USART1_Init(115200);

    setNodeId(&Master_Data, 0x01);
    setState(&Master_Data, Initialisation);
    setState(&Master_Data, Operational);
}

void board_app_init(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    delay_init(168);
    led_gpio_init();
    key_exti_init();
    canopen_stack_init();
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
        &SDO_TARGET_POS_NODE10000,
        &SDO_TARGET_POS_NODE10000,
        &SDO_TARGET_POS_NODE10000,
        &SDO_TARGET_POS_NODE10000
    };

    GPIO_ResetBits(LED_ACTION1_PORT, LED_ACTION1_PIN);
    GPIO_SetBits(LED_ACTION2_PORT, LED_ACTION2_PIN);

    run_action_common_prefix();
    send_frame_to_action_nodes(target_frames);
    delay_ms(ACTION_FRAME_DELAY_MS);
    send_common_frame_to_action_nodes(&SDO_GO);

    GPIO_SetBits(LED_ACTION1_PORT, LED_ACTION1_PIN);
}

static void run_action_two(void)
{
    const SDO_Frame *target_frames[ACTION_NODE_COUNT] = {
        &SDO_TARGET_POS_NODE1000,
        &SDO_TARGET_POS_NODE2000,
        &SDO_TARGET_POS_NODE5000,
        &SDO_TARGET_POS_NODE10000
    };

    GPIO_ResetBits(LED_ACTION2_PORT, LED_ACTION2_PIN);
    GPIO_SetBits(LED_ACTION1_PORT, LED_ACTION1_PIN);

    run_action_common_prefix();
    send_frame_to_action_nodes(target_frames);
    delay_ms(ACTION_FRAME_DELAY_MS);
    send_common_frame_to_action_nodes(&SDO_GO);

    GPIO_SetBits(LED_ACTION2_PORT, LED_ACTION2_PIN);
}

static void try_run_action(uint32_t exti_line)
{
    if (action_is_busy != 0) {
        return;
    }

    action_is_busy = 1;
    delay_ms(ACTION_DEBOUNCE_MS);

    if (exti_line == EXTI_Line1 && GPIO_ReadInputDataBit(KEY_ACTION1_PORT, KEY_ACTION1_PIN) == Bit_RESET) {
        run_action_one();
    }
    else if (exti_line == EXTI_Line13 && GPIO_ReadInputDataBit(KEY_ACTION2_PORT, KEY_ACTION2_PIN) == Bit_RESET) {
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
