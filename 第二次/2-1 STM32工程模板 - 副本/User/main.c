/* ============================================================
 *  STM32F103C6 + ADC0832 + 4位数码管 + MPX4115
 *  ADC0832:   CS=PA0, DI/DO=PA1, CLK=PA2
 *  位选:      PB0~PB3  -> 数码管公共端(低有效)
 *  段选:      PB4~PB11 -> a b c d e f g dp
 * ============================================================ */

#include "stm32f10x.h"

// 简单的微秒延时
void delay_us(u32 nus)
{
    u32 i;
    while (nus--)
    {
        for (i = 0; i < 8; i++);
    }
}

// 毫秒延时
void delay_ms(u32 nms)
{
    while (nms--)
    {
        delay_us(1000);
    }
}

/* 共阳数码管字段码 */
u8 dispcode[11] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff};

/* 位选码：低电平有效 (对应 PB0-PB3，经反相器后选通) */
u8 dispbitcode[4] = {0xF7,0xFB,0xFD,0xFE};

u8 dispbuf[4];

/* ===================== 数码管 GPIO 初始化 ===================== */
void Disp_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

    // 关键：关闭 JTAG，释放 PB3 和 PB4 作为普通 IO 使用
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    // PB0-PB3 位选
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    // PB4-PB11 段选
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 |
                                 GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_SetBits(GPIOB, 0x0FFF); // 初始消隐
}

/* ===================== 数码管动态扫描 ===================== */
void display(void)
{
    u8 k, seg_data;

    for (k = 0; k < 4; k++)
    {
        GPIOB->ODR |= 0x0FFF;    /* 消隐 */

        seg_data = dispcode[dispbuf[k]];
        if (k == 1 && dispbuf[k] != 10)
            seg_data &= ~(1 << 7);   /* 第 2 位点亮小数点 */

        GPIOB->ODR = (GPIOB->ODR & 0xFFFFF000)
                   | ((seg_data << 4) & 0x0FF0)
                   | (dispbitcode[k] & 0x0F);

        delay_ms(1); // 软件延时1ms，替代定时器
    }
}

/* ===================== ADC0832 引脚模式切换 ===================== */
void ADD_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_1;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void ADD_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_1;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* ===================== ADC0832 初始化 ===================== */
void ADC0832_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // PA0(CS), PA2(CLK) 推挽输出
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_2;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    ADD_OUT(); // PA1(DI/DO) 初始推挽输出

    GPIO_SetBits(GPIOA, GPIO_Pin_0);
    GPIO_ResetBits(GPIOA, GPIO_Pin_2);
    GPIO_SetBits(GPIOA, GPIO_Pin_1);
}

/* ===================== ADC0832 读取 ===================== */
u8 adc0832(u8 channel)
{
    u8 i, dat = 0;

    ADD_OUT();
    GPIO_SetBits(GPIOA, GPIO_Pin_1);
    GPIO_ResetBits(GPIOA, GPIO_Pin_2);
    GPIO_ResetBits(GPIOA, GPIO_Pin_0);
    delay_us(5);

    /* 第1个时钟：起始位 1 */
    GPIO_SetBits(GPIOA, GPIO_Pin_1); delay_us(5);
    GPIO_SetBits(GPIOA, GPIO_Pin_2); delay_us(5);
    GPIO_ResetBits(GPIOA, GPIO_Pin_2); delay_us(5);

    /* 第2个时钟：SGL=1 */
    GPIO_SetBits(GPIOA, GPIO_Pin_1); delay_us(5);
    GPIO_SetBits(GPIOA, GPIO_Pin_2); delay_us(5);
    GPIO_ResetBits(GPIOA, GPIO_Pin_2); delay_us(5);

    /* 第3个时钟：ODD=通道 */
    if (channel == 0) GPIO_ResetBits(GPIOA, GPIO_Pin_1);
    else              GPIO_SetBits(GPIOA, GPIO_Pin_1);
    delay_us(5);
    GPIO_SetBits(GPIOA, GPIO_Pin_2); delay_us(5);
    GPIO_ResetBits(GPIOA, GPIO_Pin_2); delay_us(5);

    /* 释放数据线，切为输入 */
    GPIO_SetBits(GPIOA, GPIO_Pin_1);
    delay_us(5);
    ADD_IN();
    delay_us(5);

    /* 2 个空时钟，让 ADC0832 把 MSB 输出到 DO */
    GPIO_SetBits(GPIOA, GPIO_Pin_2); delay_us(5);
    GPIO_ResetBits(GPIOA, GPIO_Pin_2); delay_us(5);
    GPIO_SetBits(GPIOA, GPIO_Pin_2); delay_us(5);
    GPIO_ResetBits(GPIOA, GPIO_Pin_2); delay_us(5);

    /* 读 8bit：先读，再发时钟 */
    for (i = 0; i < 8; i++)
    {
        dat <<= 1;
        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1))
            dat |= 0x01;

        GPIO_SetBits(GPIOA, GPIO_Pin_2); delay_us(5);
        GPIO_ResetBits(GPIOA, GPIO_Pin_2); delay_us(5);
    }

    GPIO_SetBits(GPIOA, GPIO_Pin_0);
    GPIO_ResetBits(GPIOA, GPIO_Pin_2);
    ADD_OUT();
    GPIO_SetBits(GPIOA, GPIO_Pin_1);

    return dat;
}

/* ===================== 主程序 ===================== */
int main(void)
{
    u8  getdata;
    u16 temp;
    float press;
    u8  nextbuf[4];

    Disp_GPIO_Init();
    ADC0832_GPIO_Init();

    while (1)
    {
        getdata = adc0832(0);

        /* ADC 值 13~243 对应压力 17~117 kPa (沿用你原公式的 17.0) */
        if (getdata >= 13 && getdata <= 243)
        {
            press = ((float)(getdata - 13) * 100.0f / 230.0f) + 15.0f;
            temp  = (u16)(press * 10.0f + 0.5f);

            nextbuf[3] = temp / 1000;          /* 百位 */
            nextbuf[2] = (temp % 1000) / 100;  /* 十位 */
            nextbuf[1] = (temp % 100) / 10;    /* 个位（带小数点） */
            nextbuf[0] = temp % 10;            /* 十分位 */

            /* 前导零熄灭 */
            if (nextbuf[3] == 0)
                dispbuf[3] = 10;
            else
                dispbuf[3] = nextbuf[3];

            dispbuf[2] = nextbuf[2];
            dispbuf[1] = nextbuf[1];
            dispbuf[0] = nextbuf[0];
        }
        else
        {
            /* 越界熄灭 */
            dispbuf[0] = 10;
            dispbuf[1] = 10;
            dispbuf[2] = 10;
            dispbuf[3] = 10;
        }

        display();
    }
}