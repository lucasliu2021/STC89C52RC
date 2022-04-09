#include <reg52.h>
#define uchar unsigned char
void Timer0_Init();  // PWM波定时器
void Timer1_Init();  //串口
void Timer2_Init();  //超时检测定时
void make();         //数据处理
void resettime();    //超时检测
void send();//响应信息
sfr T2MOD = 0xC9;    //定时器2寄存器地址
sbit LED1 = P0 ^ 0;
sbit LED2 = P0 ^ 1;
sbit LED3 = P0 ^ 2;
sbit LED4 = P0 ^ 3;
sbit LED5 = P0 ^ 6;                                  //数据灯
sbit LED6 = P0 ^ 7;                                  //计时灯
uchar level[4] = {0X00, 0X00, 0X00, 0X00}, mesg[6];  //占空比level+协议变量mesg
uchar code Fback[2][8] = {"SUCCED!","ERROR!"};
uchar time, a;
static bit uart_flag, error_flag;  //使用位变量节省空间
static uchar count;  //串口接收计数的静态变量初始值为0
//主函数
void main() {
    Timer0_Init();
    Timer1_Init();
    Timer2_Init();
    while (1) {
        send();
    }
}
//定时器0中断固定脉宽PWM输出
void Timer0() interrupt 1 {
    TR0 = 0;  //赋初值时，关闭定时器
    TH0 = 0XFF;
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
    RI = 0;  //手动清寄存器（数据接收完成自动置1）
    mesg[count] = SBUF;
    //同时判断count跟收到的数据
    if (count == 0 && mesg[count] == 0xAA) {
        count = 1;
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
        count = 0;  //判断不满足条件就将计数值清零
        error_flag = 1;  //头数据不通过
    }
    LED5 = ~LED5;
    resettime();  //进入监控
}
//超时中断50ms*400==20s
void T2_Time() interrupt 5 {
    static unsigned short run;
    TF2 = 0;  //手动标志位清零
    LED6 = 0;
    run++;
    if (run == 400) {
        if (uart_flag != 1) {
            error_flag = 1;  //时间到未完整传输，发送失败提示
        }
        count = 0;  //清零
        LED6 = 1;   //关闭状态指示灯
        LED5 = 1;  //关闭数据传输灯
        run = 0;
        TR2 = 0;  //只要计时到关闭监控，等待resettime()下一次开启
    }
}


//定时器0初始化
void Timer0_Init() {
    TMOD |= 0x01;  //定时器0工作方式1
    TH0 = 0XFF;
    TL0 = 0Xb8;
    ET0 = 1;  //开定时器0中断
    TR0 = 1;  //启动定时器0
    EA = 1;   //开总中断
}
//定时器1初始化
void Timer1_Init() {
    SCON = 0x50;  // sm0sm1=01，串口工作方式1，10位=8为数据+1位开始+1位停止
    TMOD |= 0x20;
    TH1 = 0xfd;  //波特率9600
    TL1 = 0xfd;
    TR1 = 1;
    ES = 1;  //串口使能
    PS = 1;  //串口中断优先级
    EA = 1;
}
//定时器2初始化50ms
void Timer2_Init() {
    T2MOD = 0;   //初始化模式寄存器
    T2CON = 0;   //初始化控制寄存器
    TH2 = 0x4C;  //设置定时初始值50ms
    TL2 = 0x00;
    RCAP2H = 0x4C;  //设置定时重载值
    RCAP2L = 0x00;  //设置定时重载值
    TR2 = 0;        //定时器2默认关闭
    ET2 = 1;        //开定时器2中断
    EA = 1;
}
void send(){
	    uchar i;
    //成功反馈(发送+处理)
        if (uart_flag == 1) {
            ES = 0;
            for (i = 0; i < 8; i++) {
                SBUF = Fback[0][i];
                while (!TI) ;
                TI = 0;  //发送中断请求标志位
            }
            ES = 1;
            uart_flag = 0;
            make();  //处理数据
        }
        //失败反馈（头尾检测不通过或者超时）
        if (error_flag == 1) {
            ES = 0;
            for (i = 0; i < 8; i++) {
                SBUF = Fback[1][i];
                while (!TI) ;
                TI = 0;  //发送中断请求标志位
            }
            ES = 1;
            error_flag = 0;  //恢复状态
        }
}
//数据处理（占空比赋值）
void make() {
    for (a = 0; a < 4; a++) {
        ET0 = 0;  //关定时器0
        level[a] = mesg[a + 1];
        ET0 = 1;  //开定时器0
    }
}
//超时处理开启
void resettime() {
    TR2 = 0;  //关闭定时器2
    if (count != 0) {
        TR2 = 1;  //打开定时器2
    } 
}
