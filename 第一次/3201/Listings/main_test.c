#include "stm32f10x.h"
#include "delay.h"
#include "lcd1602.h"
#include "motor.h"

int main(void)
{
    Delay_Init();
    LCD_Init();
    Motor_Init();
    LCD_ShowString(0, 0, (u8*)"MOTOR TEST       ");
    LCD_ShowString(0, 1, (u8*)"PB3=0 PB4=1      ");
    while (1)
    {
        GPIO_ResetBits(MOTOR_PORT, MOTOR_IN1_PIN);
        GPIO_SetBits(MOTOR_PORT, MOTOR_IN2_PIN);
        delay_ms(3000);
        GPIO_ResetBits(MOTOR_PORT, MOTOR_IN2_PIN);
        GPIO_SetBits(MOTOR_PORT, MOTOR_IN1_PIN);
        delay_ms(3000);
    }
}
