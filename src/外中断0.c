#include <reg52.h>
void EX0_Init();
unsigned char i = 0x00;
void main() {
    EX0_Init();
    while (1);
}
void EX0_Init() {
    EA = 1;
    EX0 = 1;  //外中断0开
    IT0 = 1;  //边沿触发
}
void power() interrupt 0 {
    
	DelayMs(50);//按键消抖

}