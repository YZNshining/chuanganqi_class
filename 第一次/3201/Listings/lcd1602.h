#ifndef __LCD1602_H
#define __LCD1602_H
#include "stm32f10x.h"

#define LCD_RS_PIN GPIO_Pin_0
#define LCD_RS_PORT GPIOB
#define LCD_RS_RCC RCC_APB2Periph_GPIOB

#define LCD_EN_PIN GPIO_Pin_2
#define LCD_EN_PORT GPIOB
#define LCD_EN_RCC RCC_APB2Periph_GPIOB

#define LCD_RW_PIN GPIO_Pin_1
#define LCD_RW_PORT GPIOB
#define LCD_RW_RCC RCC_APB2Periph_GPIOB

#define LCD_DATA_PORT GPIOA
#define LCD_DATA_RCC RCC_APB2Periph_GPIOA

void LCD_Init(void);
void LCD_Write_Cmd(u8 cmd);
void LCD_Write_Data(u8 dat);
void LCD_Clear(void);
void LCD_ShowString(u8 x,u8 y,u8 *str);

#endif
