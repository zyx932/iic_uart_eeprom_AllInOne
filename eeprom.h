#ifndef __EEPROM_H__
#define __EEPROM_H__

#include <intrins.h>
#include <STC15F104E.H>
#define device_address_read 0xA1
#define device_address_write 0xA0
typedef unsigned char u8;

//根据情况看要不要开漏输出，即你接没接上拉电阻。
//如果有自定义scl,sda线的需求，不仅要改这里还要改setup_eeprom()函数设置开漏输出的地方，见   // P3.2(SDA)和P3.3(SCL)设为开漏模式，，，，提示//TODO
sbit scl = P3^3;
sbit sda = P3^2;
sbit tx = P3^1;
sbit rx = P3^0;
u8 data_get_form_uart = 0x00 ;
bit flag=0;

void Delay15ms(){  //软件延时用于等待EEPROM完成写操作
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 1;
	j = 162;
	k = 89;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

void setup_eeprom(){

		//P3M1 |= 0x0C; // P3.2(SDA)和P3.3(SCL)设为开漏模式
    //P3M0 |= 0x0C;    //TODO

    EA=1;
    ET0=1;
    AUXR |= 0x80;		//定时器时钟1T模式
    TMOD &= 0xF0;		//设置定时器模式
    TL0 = 0x8D;		//设置定时初值
    TH0 = 0xFB;		//设置定时初值
    TF0 = 0;		//清除TF0标志
    TR0 = 0;		//定时器0不计时
}

void handle_timerZreo() interrupt 1{ //对timer0处理实现延时
    TL0 = 0x8D;		//设置定时初值
    TH0 = 0xFB;		//设置定时初值
    TF0 = 0;		//清除TF0标志
    TR0 = 0;		//定时器0停止计时
    flag = 1;       //让next_byte结束
}

void next_byte(){  //9600波特率等待104us
    TR0 = 1;		//定时器0开始计时
    while(!flag);
    flag=0;         //恢复flag
}

void uart_write(u8 datas){
    u8 i ;
    tx = 0;//开始信号
    next_byte();
    for (i = 0; i < 8 ; i++) {
        tx=(datas & 0x01);
        datas=datas>>1;
        next_byte();
    }
		tx = 1; // 停止位
		next_byte();
}

u8 uart_read(){
    u8 i;
    data_get_form_uart = 0x00;
    while(rx);//等待rx拉低即等待启示信号
    next_byte();
    for (i = 0; i < 8 ; i++) {
        data_get_form_uart = data_get_form_uart >> 1;
        if (rx) {
            data_get_form_uart |= 0x80;  // 接收数据
        }
        next_byte();
    }
    return data_get_form_uart;
}



void iic_start(){
    scl=1;
    sda=0;//start
    scl=0;
    sda=1;
}

void iic_stop(){
    sda=0;
    scl=1;
    sda=1;
}

void iic_send_bit(bit theBit){
		scl=0;
    sda=theBit;
    scl=0;
    scl=1;
    scl=0;
}

void iic_send(u8 bytes){
        int i=0;
        for(i=0;i<8;i++){
            sda=((0x80>>i) & bytes)==0? 0 : 1;
            scl=1;
            scl=0;
        }
}

u8 IIC_receive(){
    u8 tmp=0;
    int i;
    sda=1;
    for (i=0;i<8;i++){
        scl=1;
        tmp=(tmp<<1) | sda;
        scl=0;
    }
    return tmp;
}

void iic_send_ack(){
		scl=0;
    sda=1;
    scl=1;
    scl=0;
}

bit iic_receive_ack(){
    bit ack;
    scl=1;
    ack=sda;
    scl=0;
    return ack;
}


void eeprom_write(u8 addr,u8 datas){
    iic_start();
    iic_send(device_address_write);
    iic_receive_ack();
    iic_send(addr);//发地址
    iic_receive_ack();
    iic_send(datas);//发数据
    iic_receive_ack();
    iic_stop();
		Delay15ms();  //等待EEPROM完成写操作
}

u8 eeprom_read_next_addr(){
    u8 datas_op;
    iic_start();
    iic_send(device_address_read);
    iic_receive_ack();
    datas_op = iic_receive();
		iic_send_ack();
    iic_stop();
    return datas_op;
}

u8 eeprom_read(u8 addr){
		u8 datas_op;
    iic_start();
    iic_send(device_address_write);
    iic_receive_ack();
    iic_send(addr);//伪写操作，进行resigner的地址设置。
    iic_receive_ack();
    iic_stop();
    datas_op = eeprom_read_next_addr();//读取当前addr中的数据
		return datas_op;
}


#endif
