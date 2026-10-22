#ifndef __MOTOR_H
#define __MOTOR_H
#include "stm32f10x.h"

#define MOTOR_IN1_PIN GPIO_Pin_3
#define MOTOR_IN2_PIN GPIO_Pin_4
#define MOTOR_PORT GPIOB
#define MOTOR_RCC RCC_APB2Periph_GPIOB

void Motor_Init(void);
void Motor_Forward(void);
void Motor_Reverse(void);
void Motor_Stop(void);

#endif
