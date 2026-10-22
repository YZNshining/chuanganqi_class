#include "lcd1602.h"
#include "delay.h"

static void LCD_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_APB2PeriphClockCmd(LCD_RS_RCC | LCD_DATA_RCC,ENABLE);

    GPIO_InitStruct.GPIO_Pin = LCD_RS_PIN | LCD_EN_PIN | LCD_RW_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LCD_RS_PORT,&GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LCD_DATA_PORT,&GPIO_InitStruct);

    /* Put the control bus in the write-idle state before power-up init. */
    GPIO_ResetBits(LCD_RS_PORT,LCD_RS_PIN);
    GPIO_ResetBits(LCD_RW_PORT,LCD_RW_PIN);
    GPIO_ResetBits(LCD_EN_PORT,LCD_EN_PIN);
}

void LCD_Write_Cmd(u8 cmd)
{
    GPIO_ResetBits(LCD_RS_PORT,LCD_RS_PIN);
    GPIO_ResetBits(LCD_RW_PORT,LCD_RW_PIN);
    GPIO_Write(LCD_DATA_PORT,cmd);
    GPIO_SetBits(LCD_EN_PORT,LCD_EN_PIN);
    delay_us(10);
    GPIO_ResetBits(LCD_EN_PORT,LCD_EN_PIN);
    /* 清屏(0x01)/归位(0x02)执行时间最长(约1.64ms)，必须等够，否则Proteus报 busy */
    if((cmd==0x01)||(cmd==0x02))
        delay_ms(10);
    else
        delay_ms(5);
}

void LCD_Write_Data(u8 dat)
{
    GPIO_SetBits(LCD_RS_PORT,LCD_RS_PIN);
    GPIO_ResetBits(LCD_RW_PORT,LCD_RW_PIN);
    GPIO_Write(LCD_DATA_PORT,dat);
    GPIO_SetBits(LCD_EN_PORT,LCD_EN_PIN);
    delay_us(10);
    GPIO_ResetBits(LCD_EN_PORT,LCD_EN_PIN);
    delay_ms(3);
}

void LCD_Init(void)
{
    LCD_GPIO_Init();
    /* HD44780 标准上电初始化序列：Proteus模型必须按此流程，否则持续报 Controller received command whilst busy */
    delay_ms(60);                /* 上电等待：VDD稳定需 >40ms */
    LCD_Write_Cmd(0x30);
    delay_ms(10);                /* 第一次功能设置，需等待 >4.1ms */
    LCD_Write_Cmd(0x30);
    delay_ms(2);
    LCD_Write_Cmd(0x30);
    delay_ms(2);
    LCD_Write_Cmd(0x38);         /* 8位数据、2行、5x7点阵 */
    LCD_Write_Cmd(0x0c);         /* 显示开、光标关、闪烁关 */
    LCD_Write_Cmd(0x06);         /* 写入后光标右移、整屏不动 */
    LCD_Write_Cmd(0x01);         /* 清屏（内部已等待10ms） */
}

void LCD_Clear(void)
{
    LCD_Write_Cmd(0x01);
}

void LCD_ShowString(u8 x,u8 y,u8 *str)
{
    u8 i=0;
    if(y==0) LCD_Write_Cmd(0x80+x);
    else LCD_Write_Cmd(0xc0+x);
    while(str[i]!='\0')
    {
        LCD_Write_Data(str[i]);
        i++;
    }
}
