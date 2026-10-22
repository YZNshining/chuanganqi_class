#include<reg51.h>
#include <math.h>

#define uchar unsigned char
#define uint  unsigned int

#define lcd_data P0           // LCD1602数据口P0
sbit DQ     = P1^7;           // DS18B20数据引脚
sbit lcd_RS = P2^0;           // LCD1602 RS
sbit lcd_RW = P2^1;           // LCD1602 RW
sbit lcd_EN = P2^2;           // LCD1602 EN
sbit PWM    = P3^7;           // PWM电机控制引脚
sbit D      = P3^6;           // 电机转向选择位

uchar t[2], speed, temperature;
uchar DS18B20_is_ok;

// 温度显示缓存
uchar TempBuffer1[12]={0x20, 0x20, 0x20, 0x20, 0xdf, 0x43, '\0'};
// 显示字符串 "  T Monitor  "
uchar tab[16]={0x20,0x20,0x20,0x54,0x20,0x4d,0x6f,0x6e,0x69,0x74,0x6f,0x72,'\0'};

/************************ LCD1602延时子程序 ************************/
void delay_20ms(void)      // 延时20ms
{
    uchar i, temp;
    for(i = 20; i > 0; i--)
    {
        temp = 248;
        while(--temp);
        temp = 248;
        while(--temp);
    }
}

void delay_38us(void)      // 延时38us
{
    uchar temp;
    temp = 18;
    while(--temp);
}

void delay_1520us(void)    // 延时1520us
{
    uchar i, temp;
    for(i = 3; i > 0; i--)
    {
        temp = 252;
        while(--temp);
    }
}

/************************ LCD1602底层驱动 ************************/
uchar lcd_rd_status(void)         // 读取LCD状态，判忙
{
    uchar tmp_sts;
    lcd_data = 0xff;
    lcd_RW = 1;
    lcd_RS = 0;
    lcd_EN = 1;
    tmp_sts = lcd_data;
    lcd_EN = 0;
    lcd_RW = 0;
    return tmp_sts;
}

void lcd_wr_com(uchar command)    // LCD写命令
{
    while(0x80 & lcd_rd_status()); // 等待LCD不忙
    lcd_RW = 0;
    lcd_RS = 0;
    lcd_data = command;
    lcd_EN = 1;
    lcd_EN = 0;
}

void lcd_wr_data(uchar sjdata)    // LCD写显示数据
{
    while(0x80 & lcd_rd_status()); // 等待LCD不忙
    lcd_RW = 0;
    lcd_RS = 1;
    lcd_data = sjdata;
    lcd_EN = 1;
    lcd_EN = 0;
    lcd_RS = 0;
}

void Init_lcd(void)               // LCD1602初始化
{
    delay_20ms();
    lcd_wr_com(0x38);             // 16*2显示，5*8点阵，8位接口
    delay_38us();
    lcd_wr_com(0x0c);             // 开显示，关闭光标
    delay_38us();
    lcd_wr_com(0x01);             // 清屏
    delay_1520us();
    lcd_wr_com(0x06);             // 数据写入后光标自动右移
}

void GotoXY(uchar x, uchar y)     // 设置显示坐标 x列，y行
{
    if(y == 0)
        lcd_wr_com(0x80 | x);     // 第一行
    if(y == 1)
        lcd_wr_com(0xc0 | x);     // 第二行
}

void Print(uchar *str)            // 输出字符串
{
    while(*str != '\0')
    {
        lcd_wr_data(*str);
        str++;
    }
}

void LCD_Print(uchar x, uchar y, uchar *str) // 在指定坐标打印字符串
{
    GotoXY(x, y);
    Print(str);
}

/************************ 温度数据转换函数 ************************/
void covert1(void)
{
    uchar x=0x00;
    if(t[1]>0x07)                  // 判断负温度
    {
        TempBuffer1[0]=0x2d;       // '-'符号
        t[1]=~t[1];
        t[0]=~t[0];
        x=t[0]+1;
        t[0]=x;
        if(x>255)
            t[1]++;
    }
    else
    {
        TempBuffer1[0]=0x2b;       // '+'符号
    }

    t[1]<<=4;
    t[1]=t[1]&0x70;
    x=t[0];
    x>>=4;
    x=x&0x0f;
    t[1]=t[1]|x;
    temperature=t[1];

    TempBuffer1[1]=t[1]/100+0x30;
    if(TempBuffer1[1]==0x30)
        TempBuffer1[1]=0xfe;       // 百位消隐
    TempBuffer1[2]=(t[1]%100)/10+0x30;
    TempBuffer1[3]=(t[1]%100)%10+0x30;
}

/************************ DS18B20驱动函数 ************************/
void delay_18B20(uint i)
{
    while(i--);
}

void Init_DS18B20(void)           // DS18B20初始化
{
    uchar x=0;
    DQ = 1;
    delay_18B20(8);
    DQ = 0;
    delay_18B20(80);
    DQ = 1;
    delay_18B20(14);
    x=DQ;
    delay_18B20(20);
}

uchar ReadOneChar(void)           // DS18B20读1字节
{
    unsigned char i=0;
    unsigned char dat0 = 0;
    for (i=8; i>0; i--)
    {
        DQ = 0;
        dat0>>=1;
        DQ = 1;
        if(DQ)
            dat0|=0x80;
        delay_18B20(4);
    }
    return(dat0);
}

void WriteOneChar(uchar dat1)     // DS18B20写1字节
{
    uchar i=0;
    for (i=8; i>0; i--)
    {
        DQ = 0;
        DQ = dat1&0x01;
        delay_18B20(5);
        DQ = 1;
        dat1>>=1;
    }
}

void ReadTemperature(void)        // 读取DS18B20温度
{
    Init_DS18B20();
    WriteOneChar(0xCC);           // 跳过ROM
    WriteOneChar(0x44);           // 启动温度转换
    delay_18B20(80);

    Init_DS18B20();
    WriteOneChar(0xCC);
    WriteOneChar(0xBE);           // 读取温度寄存器
    delay_18B20(80);
    t[0]=ReadOneChar();           // 温度低字节
    t[1]=ReadOneChar();           // 温度高字节
}

/************************ 电机PWM调速子程序 ************************/
void delay_motor(uchar i)
{
    uchar j, k;
    for(j=i;j>0;j--)
        for(k=200;k>0;k--);
}

void motor(uchar tmp)
{
    uchar x;
    if(TempBuffer1[0]==0x2b)      // 正温度
    {
        if(tmp<25)                // 小于25度电机停止
        {
            D=0;
            PWM=0;
        }
        else if(tmp>50)           // 大于50度全速
        {
            D=0;
            PWM=1;
            x=250;
            delay_motor(x);
            PWM=0;
            x=5;
            delay_motor(x);
        }
        else                       // 25~50度PWM调速
        {
            D=0;
            PWM=1;
            x=5*tmp;
            delay_motor(x);
            PWM=0;
            x=255-5*tmp;
            delay_motor(x);
        }
    }
    else if(TempBuffer1[0]==0x2d) // 负温度电机反转
    {
        D=1;
        PWM=0;
        x=5*tmp;
        delay_motor(x);
        PWM=1;
        x=255-5*tmp;
        delay_motor(x);
    }
}

void delay(unsigned int x)
{
    unsigned char i;
    while(x--)
    {
        for(i=0; i<123; i++);
    }
}

/************************ main主函数 ************************/
void main(void)
{
    delay_20ms();
    ReadTemperature();
    Init_lcd();
    LCD_Print(0, 0, tab);
    delay(1000);

    while(1)
    {
        ReadTemperature();
        delay_18B20(100);
        covert1();
        LCD_Print(4, 1, TempBuffer1);
        motor(temperature);
    }
}