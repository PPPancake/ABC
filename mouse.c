//鼠标

#include "bootpack.h"

struct FIFO8 mousefifo;

void inthandler2c(int *esp) { //用于INT 0x2c的中断处理程序
	//鼠标的中断号码是IRQ12
	unsigned char data;
	io_out8(PIC1_OCW2, 0x64); //通知PIC(从PIC)IRQ12受理完毕，PIC持续监控
	io_out8(PIC0_OCW2, 0x62);//通知主PIC
	data = io_in8(PORT_KEYDAT);
	fifo8_put(&mousefifo, data);//缓冲区内存入数据
	return;
}

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4 // 利用鼠标模式

void enable_mouse(struct MOUSE_DEC *mdec) {
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	//顺利的话，ACK（0xfa会被传送过来）
	mdec->phase = 0;//等待0xfa的阶段
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat) {
	if(mdec->phase == 0) {//等待鼠标0xfa的阶段
		if(dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if(mdec->phase == 1) {//等待第一个字节的阶段
		if((dat & 0xc8) == 0x08) {//确保0-3 8-F的范围
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}		
		return 0;
	}
	if(mdec->phase == 2) {//等待第二个字节的阶段
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if(mdec->phase == 3) {//等待第三个字节的阶段
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;//鼠标状态是低三位的值
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}
		if((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;
		}
		mdec->y = -mdec->y;
		return 1;//!0三个字节凑齐
	}
	return -1;//出错
}
