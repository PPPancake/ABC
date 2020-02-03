#include "bootpack.h"
#include <stdio.h>

struct MOUSE_DEC {
	unsigned char buf[3], phase; //buf：3个字节，phase：当前状态
	int x,y,btn;
};

extern struct FIFO8 keyfifo, mousefifo;
void enable_mouse(struct MOUSE_DEC *mdec);
void init_keyboard(void);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

void HariMain(void) {
	//char *p; 声明的是p，*p相当于汇编中BYTE[p]，*p不是变量，p是地址变量
	struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;
	struct MOUSE_DEC mdec;
	
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
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);//描画鼠标
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	enable_mouse(&mdec);
	
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
				if (mouse_decode(&mdec, i) != 0) {
					// 3个字节凑齐了，显示出来
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) {
						s[1] = 'L';
					}
					if ((mdec.btn & 0x02) != 0) {
						s[3] = 'R';
					}
					if ((mdec.btn & 0x04) != 0) {
						s[2] = 'C';
					}
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
					//鼠标指针的移动
					boxfill8(binfo->vram,binfo->scrnx,COL8_008484,mx,my,mx+15,my+15);//隐藏鼠标
					mx += mdec.x;
					my += mdec.y;
					if(mx < 0) {
						mx = 0;
					}
					if(my < 0) {
						my = 0;
					}
					if(mx > binfo->scrnx - 16) {
						mx = binfo->scrnx - 16;
					}
					if(my > binfo->scrny - 16) {
						my = binfo->scrny - 16;
					}
					sprintf(s, "(%3d, %3d)", mx, my);
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15); //隐藏坐标
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s); //显示坐标
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16); //描画鼠标
				}
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
