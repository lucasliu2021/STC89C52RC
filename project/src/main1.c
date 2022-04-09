#include <DelayMs.h>
#include <reg52.h>
#define uchar unsigned char
void Timer0_Init();
void UART_Init();
sbit LED1 = P0 ^ 0;
sbit LED2 = P0 ^ 1;
sbit LED3 = P0 ^ 2;
sbit LED4 = P0 ^ 3;
sbit LED5 = P0 ^ 6;  //数据灯
sbit LED6 = P0 ^ 7;  //计时灯
uchar level[4] = {0XFF, 0XFF, 0XFF, 0XFF},
      mesg[6] = {0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF},  //定义协议变量mesg
			code warn[] = "ERROR!";
uchar time, i, a;
bit uart_flag, mesg_flag, error_flag;  //使用位变量节省空间
static uchar count;  //串口接收计数的静态变量初始值为0
//定时器0初始化
void Timer0_Init() {
    TMOD |= 0x01;  //定时器0工作方式1
    TH0 = 0X00;    //(65536-10000)/256;//赋初值定时20ms
    TL0 = 0Xb8;
    ET0 = 1;  //开定时器0中断
    TR0 = 1;  //启动定时器0
    EA = 1;   //开总中断
}
//定时器1初始化
void UART_Init() {
    SCON = 0x50;  // sm0sm1=01，串口工作方式1，10位=8为数据+1位开始+1位停止
    TMOD |= 0x20;
    TH1 = 0xfd;  //波特率9600
    TL1 = 0xfd;
    TR1 = 1;
    ES = 1;  //串口使能
    PS = 1;  //串口中断优先级
}
//主函数
void main() {
    Timer0_Init();
    UART_Init();
    while (1) {
        //接收完成开始反馈
        if (uart_flag == 1) {
            ES = 0;
            for (i = 0; i < 6; i++) {
                SBUF = mesg[i];
                while (!TI);
                TI = 0;//发送中断请求标志位
            }
            ES = 1;
            uart_flag = 0;
            //开始处理数据设置占空比
            for (a = 0; a < 4; a++) {
								//ET0 = 0;  //关定时器0
                level[a] = mesg[a + 1];
								//ET0 = 1;  //开定时器0
            }
        }
        /***************************
        超时检测，头正确时开始计时未完成一帧，则需重新传输
        缺点会导致发送以及数据处理时间耽搁
        LED5用于显示计时状态
        **************************/
        if (mesg_flag = 1 && count != 0) {
            LED5 = 0;
            //提示正在计时
            DelayMs(40000);
            LED5 = 1;
            count = 0;      //计数器清零
            error_flag = 1; //进入超时反馈
            mesg_flag = 0;  //计时结束
        }
        //头尾或者超时反馈
        if (error_flag==1){
            ES=0;
             for (i = 0; i < 6; i++) {
                SBUF = warn[i];
                while (!TI);
                TI = 0;//发送中断请求标志位
            }
            ES = 1;
            error_flag = 0;//恢复状态
        }
    }
}
//定时器0中断固定脉宽PWM输出
void Timer0() interrupt 1 {
    TR0 = 0;     //赋初值时，关闭定时器
    TH0 = 0X00;  //(65536-10000)/256;//赋初值定时20ms
    TL0 = 0Xb8;
    TR0 = 1;  //打开定时器
    time++;
    //占空比light
    if (time < level[0]) {
        LED1 = 0;
    } else
        LED1 = 1;
    if (time < level[1]) {
        LED2 = 0;
    } else
        LED2 = 1;
    if (time < level[2]) {
        LED3 = 0;
    } else
        LED3 = 1;
    if (time < level[3]) {
        LED4 = 0;
    } else
        LED4 = 1;
    //清零time
    if (time == 0xff) {
        time = 0x00;
    }
}

//串口中断，先接收8个字节至mesg数组保存，最后在主函数处理数据
void UART_SER() interrupt 4 {
    RI = 0;  //手动清寄存器
    mesg[count] = SBUF;
    //同时判断count跟收到的数据
    if (count == 0 && mesg[count] == 0xAA) {
        count = 1;
        mesg_flag = 1;  //开始计时标志，为1时在主程序中计时，然后清零
    } else if (count == 1) {
        count = 2;
    } else if (count == 2) {
        count = 3;
    } else if (count == 3) {
        count = 4;
    } else if (count == 4) {
        count = 5;
    } else if (count == 5 && mesg[count] == 0x55)  //检验固定的帧尾
    {
        count = 0;
        uart_flag = 1;  //串口接收成功标志，为1时在主程序中回复，然后清零
        ES = 0;  //关中断，回复完了再ES=1;
    } else {
        count = 0;       //判断不满足条件就将计数值清零
        error_flag = 1;  //进入错误反馈
    }

    LED6 = ~LED6;
}