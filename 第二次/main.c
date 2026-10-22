#include <reg51.h>       // 单片机 SFR 头文件
#include <intrins.h>     // 内部函数头文件
#define uint unsigned int
#define uchar unsigned char

// ADC0832 的引脚
sbit ADCS  = P2^0;       // ADC0832 片选
sbit ADIO  = P3^7;       // DI DO 共用一根IO，统一叫ADIO
sbit ADCLK = P3^6;       // ADC0832 时钟

// 位扫描代码，控制显示位
unsigned char code dispbitcode[8] = {
    0xf7, 0xfb, 0xfd, 0xfe, 0xef, 0xdf, 0xbf, 0x7f
};
// 共阳数码管字段码，最后一个是熄灭
unsigned char code dispcode[11] = {
    0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92,
    0x82, 0xF8, 0x80, 0x90, 0xff
};
volatile unsigned char dispbuf[4]; // 显示缓冲区
uchar getdata;                     // ADC 转换后的数据
volatile unsigned char scan_index = 0;

// Timer0 中断：每 1ms 扫描一位数码管
void timer0_scan(void) interrupt 1 using 1
{
    unsigned char segments;
    TH0 = 0xfc;
    TL0 = 0x18;
    P1 = 0xff;                         // 先消隐
    segments = dispcode[dispbuf[scan_index]];
    // 第 2 位点亮小数点，且该位不是熄灭状态
    if (scan_index == 1 && dispbuf[scan_index] != 10)
        segments &= 0x7f;
    P0 = segments;
    P1 = dispbitcode[scan_index];
    scan_index++;
    if (scan_index == 4) scan_index = 0;
}

void display_init(void)
{
    P1 = 0xff;
    P0 = 0xff;
    TMOD = (TMOD & 0xf0) | 0x01; // T0 方式 1
    TH0 = 0xfc;
    TL0 = 0x18;
    ET0 = 1;
    EA = 1;
    TR0 = 1;
}

/************ 修复 ADC0832 函数 ************/
unsigned char adc0832(unsigned char channel)
{
    unsigned char i;
    unsigned char dat = 0;
    unsigned char cfg;
    // CH0:1 1 0，CH1:1 1 1
    if (channel == 0)
        cfg = 0x02; // 10
    else
        cfg = 0x03; //11

    ADIO = 1;
    ADCLK = 0;
    ADCS = 0;
    _nop_();_nop_();_nop_();_nop_(); // 等待ADC稳定，增加延时

    //起始位1
    ADIO = 1;
    _nop_();_nop_();
    ADCLK = 1; _nop_();_nop_();
    ADCLK = 0; _nop_();_nop_();

    //SGL
    ADIO = (cfg >> 1) & 0x01;
    _nop_();_nop_();
    ADCLK = 1; _nop_();_nop_();
    ADCLK = 0; _nop_();_nop_();

    //ODD
    ADIO = cfg & 0x01;
    _nop_();_nop_();
    ADCLK = 1; _nop_();_nop_();
    ADCLK = 0; _nop_();_nop_();

    ADIO = 1; //释放数据线，切换输入
    _nop_();_nop_();

    //读取8bit MSB在前
    for (i = 0; i < 8; i++)
    {
        ADCLK = 1;
        _nop_();_nop_();
        ADCLK = 0;
        _nop_();_nop_();
        dat <<= 1;
        if (ADIO)
            dat |= 0x01;
    }
    ADCS = 1;
    ADCLK = 0;
    ADIO = 1;
    return dat;
}

// 主程序
void main(void)
{
    unsigned int temp;
    uchar nextbuf[4];
    float press;
    display_init();
    while (1)
    {
        getdata = adc0832(0); //读ADC，通道0

        // ADC 值 13~243 对应压力 15~115 kPa
        if (getdata >= 13 && getdata <= 243)
        {
            // press = 15 + (getdata - 13) * (115 - 15) / (243 - 13)
            press = ((float)(getdata - 13) * 100.0 / 230.0) + 17.0;
            temp = (unsigned int)(press * 10.0 + 0.5); // 放大 10 倍并四舍五入

            nextbuf[3] = temp / 1000;          // 百位
            nextbuf[2] = (temp % 1000) / 100;  // 十位
            nextbuf[1] = (temp % 100) / 10;    // 个位
            nextbuf[0] = temp % 10;            // 十分位

            // 处理前导零：百位为0时熄灭
            if (nextbuf[3] == 0) {
                dispbuf[3] = 10; // 熄灭百位
            } else {
                dispbuf[3] = nextbuf[3];
            }
            dispbuf[2] = nextbuf[2];
            dispbuf[1] = nextbuf[1];
            dispbuf[0] = nextbuf[0];
        }
        else
        {
            //越界熄灭
            dispbuf[0] = 10;
            dispbuf[1] = 10;
            dispbuf[2] = 10;
            dispbuf[3] = 10;
        }
    }
}