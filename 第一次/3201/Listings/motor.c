#include "motor.h"

static void Motor_SetOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = MOTOR_IN1_PIN | MOTOR_IN2_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MOTOR_PORT,&GPIO_InitStruct);
}

void Motor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    /* PB3/PB4 are JTAG pins after reset. Release them before GPIO setup. */
    RCC_APB2PeriphClockCmd(MOTOR_RCC | RCC_APB2Periph_AFIO,ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);
    GPIO_InitStruct.GPIO_Pin = MOTOR_IN1_PIN | MOTOR_IN2_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MOTOR_PORT,&GPIO_InitStruct);
    Motor_Stop();
}

void Motor_Forward(void)
{
    Motor_SetOutput();
    // PB3=0 PB4=1 ��ת
    GPIO_ResetBits(MOTOR_PORT,MOTOR_IN1_PIN);
    GPIO_SetBits(MOTOR_PORT,MOTOR_IN2_PIN);
}
void Motor_Reverse(void)
{
    Motor_SetOutput();
    // PB3=1 PB4=0 ��ת
    GPIO_SetBits(MOTOR_PORT,MOTOR_IN1_PIN);
    GPIO_ResetBits(MOTOR_PORT,MOTOR_IN2_PIN);
}
void Motor_Stop(void)
{
    Motor_SetOutput();
    // PB3=0 PB4=0 ֹͣ����ֹͬʱ1
    GPIO_ResetBits(MOTOR_PORT,MOTOR_IN1_PIN);
    GPIO_ResetBits(MOTOR_PORT,MOTOR_IN2_PIN);
}

