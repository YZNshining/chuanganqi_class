#ifndef __DS18B20_H
#define __DS18B20_H
#include "stm32f10x.h"

#define DS_PIN GPIO_Pin_10
#define DS_PORT GPIOB
#define DS_RCC RCC_APB2Periph_GPIOB

u8 DS18B20_Init(void);
u8 DS18B20_ReadByte(void);
void DS18B20_WriteByte(u8 dat);
float DS18B20_ReadTemp(void);

#endif
