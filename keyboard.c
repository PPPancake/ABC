#include "bootpack.h"

struct FIFO8 keyfifo;

void inthandler21(int *esp) { //����INT 0x21���жϴ������
	unsigned char data;
	io_out8(PIC0_OCW2, 0x61);	/* ֪ͨPIC IRQ01������ϣ�PIC��������IRQ1 */
	data = io_in8(PORT_KEYDAT);
	fifo8_put(&keyfifo, data);//�������ڴ�������
	return;
}

#define PORT_KEYSTA				0x0064
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
