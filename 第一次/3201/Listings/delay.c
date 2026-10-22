#include "delay.h"
#include "misc.h"

void Delay_Init(void)
{
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
}

/* Proteus仿真：系统时钟=HSI内部8MHz，SysTick=8MHz/8=1MHz */
void delay_us(u32 nus)
{
    u32 temp;
    SysTick->LOAD = nus*1;   /* 1us = 1 tick @1MHz */
    SysTick->VAL=0x00;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    do
    {
        temp=SysTick->CTRL;
    }while((temp&0x01)&&!(temp&(1<<16)));
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL=0x00;
}

void delay_ms(u32 nms)
{
    u32 temp;
    SysTick->LOAD = nms*1000;   /* 1ms = 1000 ticks @1MHz */
    SysTick->VAL=0x00;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    do
    {
        temp=SysTick->CTRL;
    }while((temp&0x01)&&!(temp&(1<<16)));
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL=0x00;
}
