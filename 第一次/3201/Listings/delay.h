#ifndef __DELAY_H
#define __DELAY_H
#include "stm32f10x.h"

void Delay_Init(void);
void delay_us(u32 nus);
void delay_ms(u32 nms);

#endif
