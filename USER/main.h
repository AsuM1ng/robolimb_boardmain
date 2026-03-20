#ifndef _MAIN_H
#define	_MAIN_H

#include "sys.h"
#include "delay.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "usart.h"

/*****CANopen 头文件********/
#include "can1.h"
#include "timer2.h"
#include "timer3.h"
#include "data.h"
#include "Master.h"
#include "canfestival.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"

void board_app_init(void);

#endif
