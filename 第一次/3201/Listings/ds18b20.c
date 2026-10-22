#include "ds18b20.h"
#include "delay.h"

static void DS_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_APB2PeriphClockCmd(DS_RCC,ENABLE);
    GPIO_InitStruct.GPIO_Pin = DS_PIN;
    /* 1-Wire is an open-drain bus: the MCU only pulls the line low and the
       external 4.7 kOhm resistor releases it high. */
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS_PORT,&GPIO_InitStruct);
}
static void DS_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = DS_PIN;
    /* Keep the bus released high while sampling.  The schematic already has
       R7=4.7k to 5V; the internal pull-up also makes the Proteus model robust
       against a floating transition when the pin mode is changed. */
    GPIO_SetBits(DS_PORT,DS_PIN);
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DS_PORT,&GPIO_InitStruct);
}

u8 DS18B20_Init(void)
{
    u8 presence;
    DS_OUT();
    GPIO_ResetBits(DS_PORT,DS_PIN);
    /* The 1-Wire reset pulse is measured in microseconds, not milliseconds. */
    delay_us(480);
    GPIO_SetBits(DS_PORT,DS_PIN);
    /* Release first, then immediately change to input so the sensor can pull
       the bus low for its presence pulse. */
    DS_IN();
    delay_us(40);                  /* sample inside the 15..60 us window */
    presence = GPIO_ReadInputDataBit(DS_PORT,DS_PIN);
    delay_us(410);                 /* complete the 480 us reset slot */
    return (presence == 0);
}

static u8 DS18B20_ReadBit(void)
{
    u8 bit;

    DS_OUT();
    GPIO_ResetBits(DS_PORT,DS_PIN);
    delay_us(3);                   /* initiate a read slot */
    GPIO_SetBits(DS_PORT,DS_PIN);  /* release the open-drain bus */
    DS_IN();
    delay_us(12);                  /* sample around 15 us from slot start */
    bit = GPIO_ReadInputDataBit(DS_PORT,DS_PIN);
    delay_us(48);                  /* standard 60 us slot */
    return bit ? 1 : 0;
}

u8 DS18B20_ReadByte(void)
{
    u8 i,dat=0;
    for(i=0;i<8;i++)
    {
        dat >>= 1;
        if(DS18B20_ReadBit())
            dat |= 0x80;
    }
    return dat;
}

void DS18B20_WriteByte(u8 dat)
{
    u8 i;
    DS_OUT();
    for(i=0;i<8;i++)
    {
        if(dat&0x01)
        {
            GPIO_ResetBits(DS_PORT,DS_PIN);
            delay_us(3);
            GPIO_SetBits(DS_PORT,DS_PIN);
            delay_us(57);           /* total write-1 slot: about 60 us */
        }
        else
        {
            GPIO_ResetBits(DS_PORT,DS_PIN);
            delay_us(60);           /* write-0 keeps the bus low */
            GPIO_SetBits(DS_PORT,DS_PIN);
            delay_us(5);
        }
        dat >>=1;
    }
}

float DS18B20_ReadTemp(void)
{
    u8 L,H;
    short temp;
    float ftemp;
    if(!DS18B20_Init())
        return -127.0f;
    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0x44);
    delay_ms(800);   /* 12-bit conversion: allow the full conversion time */
    if(!DS18B20_Init())
        return -127.0f;
    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0xBE);
    L=DS18B20_ReadByte();
    H=DS18B20_ReadByte();
    temp = (H<<8)|L;
    ftemp = temp*0.0625f;
    return ftemp;
}
