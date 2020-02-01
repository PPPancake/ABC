//中断
#include "bootpack.h"
#include <stdio.h>

void init_pic(void) {
	// PIC0 主PIC ，PIC1 从PIC
	io_out8(PIC0_IMR,  0xff  ); /* 禁止所有中断 */
	io_out8(PIC1_IMR,  0xff  ); /* 禁止所有中断 */

	io_out8(PIC0_ICW1, 0x11  ); /* 边沿触发模式 */
	io_out8(PIC0_ICW2, 0x20  ); /* IRQ0-7由INT20-27接收 */
	io_out8(PIC0_ICW3, 1 << 2); /* PIC1由IRQ2连接 */
	io_out8(PIC0_ICW4, 0x01  ); /* 无缓冲区模式 */

	io_out8(PIC1_ICW1, 0x11  ); /* 边沿触发模式 */
	io_out8(PIC1_ICW2, 0x28  ); /* IRQ8-15由INT28-2f接收 */
	io_out8(PIC1_ICW3, 2     ); /* PIC1由IRQ2连接 */
	io_out8(PIC1_ICW4, 0x01  ); /* 无缓冲区模式 */

	io_out8(PIC0_IMR,  0xfb  ); /* 11111011 PIC1以外全部禁止 */
	io_out8(PIC1_IMR,  0xff  ); /* 11111111 禁止所有中断 */

	return;
}

#define PORT_KEYDAT		0x0060

struct FIFO8 keyfifo;

void inthandler21(int *esp) { //用于INT 0x21的中断处理程序
	unsigned char data;
	io_out8(PIC0_OCW2, 0x61);	/* 通知PIC IRQ01受理完毕，PIC持续监视IRQ1 */
	data = io_in8(PORT_KEYDAT);
	fifo8_put(&keyfifo, data);//缓冲区内存入数据
	return;
}

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

void inthandler27(int *esp)
/* PIC0からの不完全割り込み対策 */
/* Athlon64X2機などではチップセットの都合によりPICの初期化時にこの割り込みが1度だけおこる */
/* この割り込み処理関数は、その割り込みに対して何もしないでやり過ごす */
/* なぜ何もしなくていいの？
	→  この割り込みはPIC初期化時の電気的なノイズによって発生したものなので、
		まじめに何か処理してやる必要がない。									*/
{
	io_out8(PIC0_OCW2, 0x67); /* IRQ-07受付完了をPICに通知 */
	return;
}
