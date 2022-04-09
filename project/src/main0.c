#include <reg52.h>
void Timer0_Init();
void UART_Init();
unsigned char flag, temp, i, time, code table[] = "Done!";
//主函数
void main() {
    Timer0_Init();
    UART_Init();
    while (1) {
        if (flag == 1) {
            ES = 0;
            for (i = 0; i < 6; i++) {
                SBUF = table[i];
                while (!TI) ;
                TI = 0;/发送中断请求标志位
            }
            SBUF = temp;
            while (!TI);
            TI = 0;
            ES = 1;
            flag = 0;
        }
    }
}
//定时器0中断
void Timer0() interrupt 1 {
    TR0 = 0;//赋初值时，关闭定时器
    TH0 = 0X00;
    TL0 = 0XB8;
    TR0 = 1;//打开定时器
    time++;
    if (time < temp) {
        P0 = 0X00;
    } else
        P0 = 0XFF;
    //清零time
    if (time == 0xff) {
        time = 0x00;
    }
}
void UART_SER() interrupt 4 {
        RI = 0;//标志位清零
        temp = SBUF;//读入缓冲区的值
        flag = 1;
}
//定时器0初始化
void Timer0_Init() {
    TMOD |= 0x01;//定时器0工作方式1
    TH0 = 0X00;
    TL0 = 0XB8;
    ET0 = 1;//开定时器0中断
    TR0 = 1;//启动定时器0
    EA = 1;//开总中断
}
//定时器1初始化
void UART_Init() {
    SCON = 0x50;// 01，串口工作方式1，10位=8为数据+1位开始+1位停止
    TMOD |= 0x20;
    TH1 = 0xfd;
    TL1 = 0xfd;
    TR1 = 1;
    ES = 1;//使能串口
    PS = 1;//串口中断优先级
}