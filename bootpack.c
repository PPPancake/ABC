#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyfifo, mousefifo;
void enable_mouse(void);
void init_keyboard(void);

void HariMain(void) {
	//char *p; 声明的是p，*p相当于汇编中BYTE[p]，*p不是变量，p是地址变量
	struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;
	
	init_gdtidt();
	init_pic();
	io_sti();
	
	fifo8_init(&keyfifo, 32, keybuf); //void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf)
	fifo8_init(&mousefifo, 128, mousebuf);
	io_out8(PIC0_IMR, 0xf9); //允许使用PIC1和键盘
	io_out8(PIC1_IMR, 0xef); //允许使用鼠标
	
	init_keyboard();
	
	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2;//在屏幕中间
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	enable_mouse();
	
	/*
	指针的第一种写法
	for(i = 0xa0000; i <= 0xaffff; i++) {
		p = (char *)i; //地址带入
		*p = i & 0x0f;
	}
	指针的第二种写法
	p = (char *) 0xa0000;
	for(i = 0; i <= 0xffff; i++) {
		*(p + i) = i & 0x0f;
	}
	指针的第三种
	p = (char *) 0xa0000;
	for(i = 0; i <= 0xffff; i++) {
		p[i] = i & 0x0f;
	}
	*/	
	
	for(;;) {
		io_cli();
		if(fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) { //剩余空间都为0时
			io_stihlt();
		} else {
			if (fifo8_status(&keyfifo) != 0) { //keyboard缓冲区剩余空间不为0时
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484,  0, 16, 15, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			} else if (fifo8_status(&mousefifo) != 0) {
				i = fifo8_get(&mousefifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 47, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
			}	
		}
	}
}

#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
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

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4 // 利用鼠标模式

void enable_mouse(void) {
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	return;
}
