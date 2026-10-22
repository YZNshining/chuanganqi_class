#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>

//================引脚宏定义================
#define DS18B20_PIN     GPIO_Pin_0
#define DS18B20_PORT    GPIOA
#define DS18B20_RCC     RCC_APB2Periph_GPIOA

#define LCD_RS_PIN      GPIO_Pin_1
#define LCD_RW_PIN      GPIO_Pin_2
#define LCD_EN_PIN      GPIO_Pin_3
#define LCD_CTRL_PORT   GPIOA
#define LCD_CTRL_RCC    RCC_APB2Periph_GPIOA

#define LCD_DATA_PIN    (GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7)
#define LCD_DATA_PORT   GPIOB
#define LCD_DATA_RCC    RCC_APB2Periph_GPIOB

#define MOT_IN1_PIN     GPIO_Pin_4
#define MOT_IN2_PIN     GPIO_Pin_5
#define MOT_PORT        GPIOA
#define MOT_RCC         RCC_APB2Periph_GPIOA

//=============软件延时函数 (专为 8MHz Proteus 优化)=============
void delay_us(uint32_t nus)
{
    volatile uint32_t i;
    for(i = 0; i < nus * 8; i++) {
        __NOP();
    }
}

void delay_ms(uint32_t nms)
{
    while(nms--) {
        delay_us(1000);
    }
}

//================DS18B20底层IO模式切换================
void DS18B20_Out_Mode(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin   = DS18B20_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}

void DS18B20_In_Mode(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin   = DS18B20_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IPU; // 上拉输入
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}

//============DS18B20时序函数============
uint8_t DS18B20_Reset(void)
{
    uint8_t ack;
    uint16_t timeout = 0;
    
    DS18B20_Out_Mode();
    GPIO_ResetBits(DS18B20_PORT, DS18B20_PIN);
    delay_us(500);  // 必须 >= 480us
    GPIO_SetBits(DS18B20_PORT, DS18B20_PIN);
    DS18B20_In_Mode();
    
    while(GPIO_ReadInputDataBit(DS18B20_PORT, DS18B20_PIN) == 1) {
        timeout++;
        delay_us(1);
        if(timeout > 500) {
            return 1; 
        }
    }
    
    ack = 0; 
    delay_us(200); 
    return ack;
}

void DS18B20_WriteBit(uint8_t bit_dat)
{
    DS18B20_Out_Mode();
    GPIO_ResetBits(DS18B20_PORT, DS18B20_PIN);
    delay_us(2);
    if(bit_dat) GPIO_SetBits(DS18B20_PORT, DS18B20_PIN);
    delay_us(60);
    GPIO_SetBits(DS18B20_PORT, DS18B20_PIN);
}

uint8_t DS18B20_ReadBit(void)
{
    uint8_t bit_dat;
    DS18B20_Out_Mode();
    GPIO_ResetBits(DS18B20_PORT, DS18B20_PIN);
    delay_us(2);
    GPIO_SetBits(DS18B20_PORT, DS18B20_PIN);
    DS18B20_In_Mode();
    delay_us(5);
    bit_dat = GPIO_ReadInputDataBit(DS18B20_PORT, DS18B20_PIN);
    delay_us(50);
    return bit_dat;
}

void DS18B20_WriteByte(uint8_t byte_dat)
{
    uint8_t i;
    for(i = 0; i < 8; i++) {
        DS18B20_WriteBit(byte_dat & 0x01);
        byte_dat >>= 1;
    }
}

uint8_t DS18B20_ReadByte(void)
{
    uint8_t i, byte_dat = 0;
    for(i = 0; i < 8; i++) {
        byte_dat >>= 1;
        if(DS18B20_ReadBit()) byte_dat |= 0x80;
    }
    return byte_dat;
}

void DS18B20_StartConvert(void)
{
    if(DS18B20_Reset()) return;
    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0x44);
}

int16_t DS18B20_ReadTempResult(void)
{
    uint8_t tl, th;
    int16_t temp;

    if(DS18B20_Reset()) return 0x7FFF;
    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0xBE);
    tl = DS18B20_ReadByte();
    th = DS18B20_ReadByte();

    temp = (th << 8) | tl;
    if(temp & 0x8000) { 
        temp = ~temp + 1;
        temp = -temp;
    }
    temp = temp * 10 / 16; 
    return temp;
}

//================LCD1602驱动================
void LCD1602_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_APB2PeriphClockCmd(LCD_CTRL_RCC | LCD_DATA_RCC, ENABLE);

    GPIO_InitStruct.GPIO_Pin   = LCD_RS_PIN | LCD_RW_PIN | LCD_EN_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LCD_CTRL_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin   = LCD_DATA_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LCD_DATA_PORT, &GPIO_InitStruct);
}

void LCD_WriteCmd(uint8_t cmd)
{
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_RS_PIN);
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_RW_PIN);
    LCD_DATA_PORT->ODR = (LCD_DATA_PORT->ODR & 0xFF00) | cmd;
    delay_us(100); 
    GPIO_SetBits(LCD_CTRL_PORT, LCD_EN_PIN);
    delay_us(100);
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_EN_PIN);
    delay_us(100);
}

void LCD_WriteData(uint8_t dat)
{
    GPIO_SetBits(LCD_CTRL_PORT, LCD_RS_PIN);
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_RW_PIN);
    LCD_DATA_PORT->ODR = (LCD_DATA_PORT->ODR & 0xFF00) | dat;
    delay_us(100);
    GPIO_SetBits(LCD_CTRL_PORT, LCD_EN_PIN);
    delay_us(100);
    GPIO_ResetBits(LCD_CTRL_PORT, LCD_EN_PIN);
    delay_us(100);
}

void LCD_Init(void)
{
    LCD1602_GPIO_Init();
    delay_ms(50);
    LCD_WriteCmd(0x38); 
    LCD_WriteCmd(0x0C); 
    LCD_WriteCmd(0x06); 
    LCD_WriteCmd(0x01); 
    delay_ms(5);
}

void LCD_ShowString(uint8_t x, uint8_t y, uint8_t *str)
{
    uint8_t i;
    if(y == 0) LCD_WriteCmd(0x80 + x);
    else LCD_WriteCmd(0xC0 + x);
    for(i = 0; str[i] != '\0'; i++) {
        LCD_WriteData(str[i]);
    }
}

//================电机H桥================
void Motor_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_APB2PeriphClockCmd(MOT_RCC, ENABLE);
    GPIO_InitStruct.GPIO_Pin   = MOT_IN1_PIN | MOT_IN2_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MOT_PORT, &GPIO_InitStruct);
    GPIO_ResetBits(MOT_PORT, MOT_IN1_PIN);
    GPIO_ResetBits(MOT_PORT, MOT_IN2_PIN);
}

#define MOT_STOP()      do{GPIO_ResetBits(MOT_PORT, MOT_IN1_PIN); GPIO_ResetBits(MOT_PORT, MOT_IN2_PIN);}while(0)
#define MOT_FORWARD()   do{GPIO_SetBits(MOT_PORT, MOT_IN1_PIN);   GPIO_ResetBits(MOT_PORT, MOT_IN2_PIN);}while(0)

//================主函数================
int main(void)
{
    int16_t temp = 0;
    int16_t last_temp = -999;          
    char lcd_buf[17];
    uint8_t motor_state = 0;
    uint8_t last_motor_state = 0;      

    uint8_t ds_state = 0;              
    uint16_t ds_timer = 0;
    
    // 【C89规范】必须将变量声明在代码块最前面，防止编译报错
    int len;
    int i;

    // 禁用 JTAG，释放 PB3、PB4 作为普通 IO
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    RCC_APB2PeriphClockCmd(DS18B20_RCC, ENABLE);

    LCD_Init();
    Motor_GPIO_Init();

    // 初始化屏幕提示（仅初次显示）
    LCD_ShowString(0, 0, (uint8_t*)"T Monitor       ");
    LCD_ShowString(0, 1, (uint8_t*)"Motor:Stop      ");

    // 上电先启动一次转换，等待并丢弃结果（避开85℃默认值）
    DS18B20_StartConvert();
    delay_ms(100);                     
    DS18B20_ReadTempResult();          

    while(1)
    {
        switch(ds_state)
        {
            case 0: 
                DS18B20_StartConvert();
                ds_timer = 0;
                ds_state = 1;
                break;

            case 1: 
                ds_timer++;
                if(ds_timer >= 10)         
                {
                    ds_state = 2;
                }
                break;

            case 2: 
                temp = DS18B20_ReadTempResult();
                
                if(temp == 0x7FFF) {
                    if(last_temp != 0x7FFF) {
                        LCD_ShowString(0, 0, (uint8_t*)"Sensor Error!   ");
                        MOT_STOP();
                        LCD_ShowString(0, 1, (uint8_t*)"Motor:Stop      ");
                        motor_state = 0;
                        last_temp = 0x7FFF;
                        last_motor_state = 0;
                    }
                } else {
                    if(temp != last_temp) {
                        last_temp = temp;
                        
                        // 【修改显示格式】目标效果：+29.0°C 或 -05.0°C
                        if(temp < 0) {
                            len = sprintf(lcd_buf, "-%d.%d\xDF""C", (int)((-temp) / 10), (int)((-temp) % 10));
                        } else {
                            len = sprintf(lcd_buf, "+%d.%d\xDF""C", (int)(temp / 10), (int)(temp % 10));
                        }
                        // 补空格填满整行
                        for(i = len; i < 16; i++) lcd_buf[i] = ' ';
                        lcd_buf[16] = '\0';
                        LCD_ShowString(0, 0, (uint8_t*)lcd_buf);
                    }

                    // 温度阈值控制（回差 28~30℃）
                    if(temp > 300) {        
                        if(motor_state != 1) {
                            MOT_FORWARD();
                            motor_state = 1;
                        }
                    } else if(temp < 280) { 
                        if(motor_state != 0) {
                            MOT_STOP();
                            motor_state = 0;
                        }
                    }
                    
                    if(motor_state != last_motor_state) {
                        last_motor_state = motor_state;
                        if(motor_state == 1) {
                            LCD_ShowString(0, 1, (uint8_t*)"Motor:Forward   ");
                        } else {
                            LCD_ShowString(0, 1, (uint8_t*)"Motor:Stop      ");
                        }
                    }
                }
                ds_state = 0; 
                break;
        }
        delay_ms(10); 
    }
}