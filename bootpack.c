#include "bootpack.h"
#include <stdio.h>

struct MOUSE_DEC {
	unsigned char buf[3], phase; //buf��3���ֽڣ�phase����ǰ״̬
	int x,y,btn;
};

extern struct FIFO8 keyfifo, mousefifo;
void enable_mouse(struct MOUSE_DEC *mdec);
void init_keyboard(void);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

void HariMain(void) {
	//char *p; ��������p��*p�൱�ڻ����BYTE[p]��*p���Ǳ�����p�ǵ�ַ����
	struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;
	struct MOUSE_DEC mdec;
	
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
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);//�軭���
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	enable_mouse(&mdec);
	
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
				if (mouse_decode(&mdec, i) != 0) {
					// 3���ֽڴ����ˣ���ʾ����
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
					//���ָ����ƶ�
					boxfill8(binfo->vram,binfo->scrnx,COL8_008484,mx,my,mx+15,my+15);//�������
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
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15); //��������
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s); //��ʾ����
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16); //�軭���
				}
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

void enable_mouse(struct MOUSE_DEC *mdec) {
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	//˳���Ļ���ACK��0xfa�ᱻ���͹�����
	mdec->phase = 0;//�ȴ�0xfa�Ľ׶�
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat) {
	if(mdec->phase == 0) {//�ȴ����0xfa�Ľ׶�
		if(dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if(mdec->phase == 1) {//�ȴ���һ���ֽڵĽ׶�
		if((dat & 0xc8) == 0x08) {//ȷ��0-3 8-F�ķ�Χ
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}		
		return 0;
	}
	if(mdec->phase == 2) {//�ȴ��ڶ����ֽڵĽ׶�
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if(mdec->phase == 3) {//�ȴ��������ֽڵĽ׶�
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;//���״̬�ǵ���λ��ֵ
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}
		if((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;
		}
		mdec->y = -mdec->y;
		return 1;//!0�����ֽڴ���
	}
	return -1;//����
}
