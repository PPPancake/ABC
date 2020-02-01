#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyfifo, mousefifo;
void enable_mouse(void);
void init_keyboard(void);

void HariMain(void) {
	//char *p; ��������p��*p�൱�ڻ����BYTE[p]��*p���Ǳ�����p�ǵ�ַ����
	struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;
	
	init_gdtidt();
	init_pic();
	io_sti();
	
	fifo8_init(&keyfifo, 32, keybuf); //void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf)
	fifo8_init(&mousefifo, 128, mousebuf);
	io_out8(PIC0_IMR, 0xf9); //����ʹ��PIC1�ͼ���
	io_out8(PIC1_IMR, 0xef); //����ʹ�����
	
	init_keyboard();
	
	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2;//����Ļ�м�
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	enable_mouse();
	
	/*
	ָ��ĵ�һ��д��
	for(i = 0xa0000; i <= 0xaffff; i++) {
		p = (char *)i; //��ַ����
		*p = i & 0x0f;
	}
	ָ��ĵڶ���д��
	p = (char *) 0xa0000;
	for(i = 0; i <= 0xffff; i++) {
		*(p + i) = i & 0x0f;
	}
	ָ��ĵ�����
	p = (char *) 0xa0000;
	for(i = 0; i <= 0xffff; i++) {
		p[i] = i & 0x0f;
	}
	*/	
	
	for(;;) {
		io_cli();
		if(fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) { //ʣ��ռ䶼Ϊ0ʱ
			io_stihlt();
		} else {
			if (fifo8_status(&keyfifo) != 0) { //keyboard������ʣ��ռ䲻Ϊ0ʱ
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
#define KEYCMD_WRITE_MODE		0x60 //ģʽ�趨ģʽ
#define KBC_MODE				0x47 // ���ü���ģʽ

void wait_KBC_sendready(void) { //�ȴ����̿��Ƶ�·׼�����
	for(;;) {
		if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {//���̿��Ƶ�·׼���ý���CPUָ��ʱ��0x0064λ�����ݵĵ�����2λ��0
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
#define MOUSECMD_ENABLE			0xf4 // �������ģʽ

void enable_mouse(void) {
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	return;
}
