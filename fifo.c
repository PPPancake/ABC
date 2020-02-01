// 先进先出缓冲区
#include "bootpack.h"

#define FLAGS_OVERRUN 0x0001

void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf)
{ //初始化
	fifo->size = size;
	fifo->buf = buf;//缓冲区的地址
	fifo->free = size;
	fifo->flags = 0;//记录是否溢出
	fifo->p = 0; //下一个数据写入地址
	fifo->q = 0; //下一个数据读出地址
	return;
}

int fifo8_put(struct FIFO8 *fifo, unsigned char data) {//传送一字节的数据并保存
	if(fifo->free == 0) { //空余空间< 0 设flags = 1
		fifo->flags |= FLAGS_OVERRUN;
		return -1; //溢出返回-1
	}
	fifo->buf[fifo->p] = data;
	fifo->p++; //p指向下一位置
	if(fifo->p == fifo->size) { //到最大从头开始写
		fifo->p = 0;
	}
	fifo->free--;
	return 0;
}

int fifo8_get(struct FIFO8 *fifo) {//取出一字节的内容
	int data;
	if(fifo->free == fifo->size) { //缓冲区为空返回-1
		return -1;
	}
	data = fifo->buf[fifo->q];
	fifo->q++;
	if(fifo->q == fifo->size) { //到最大从头开始读
		fifo->q = 0;
	}
	fifo->free++;
	return data;
}

int fifo8_status(struct FIFO8 *fifo) { //查看积攒了多少数据
	return fifo->size - fifo->free;
}
