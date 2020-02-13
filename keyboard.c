#include "bootpack.h"

struct FIFO8 keyfifo;

void inthandler21(int *esp) { //用于INT 0x21的中断处理程序
	unsigned char data;
	io_out8(PIC0_OCW2, 0x61);	/* 通知PIC IRQ01受理完毕，PIC持续监视IRQ1 */
	data = io_in8(PORT_KEYDAT);
	fifo8_put(&keyfifo, data);//缓冲区内存入数据
	return;
}

#define PORT_KEYSTA				0x0064
#define KEYSTA_SEND_NOTREADY	2
#define KEYCMD_WRITE_MODE		0x60 //模式设定模式
#define KBC_MODE				0x47 // 利用键盘模式

void wait_KBC_sendready(void) { //等待键盘控制电路准备完毕
	for(;;) {
		if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {//键盘控制电路准备好接受CPU指令时，0x0064位置数据的倒数第2位是0
			break;
		}
	}
	return;
}

void init_keyboard(void) {
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE); 
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT,KBC_MODE);
	return;
}
