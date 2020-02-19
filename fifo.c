// �Ƚ��ȳ�������
#include "bootpack.h"

#define FLAGS_OVERRUN 0x0001

void fifo32_init(struct FIFO32 *fifo, int size, int *buf)
{ //��ʼ��
	fifo->size = size;
	fifo->buf = buf;//�������ĵ�ַ
	fifo->free = size;
	fifo->flags = 0;//��¼�Ƿ����
	fifo->p = 0; //��һ������д���ַ
	fifo->q = 0; //��һ�����ݶ�����ַ
	return;
}

int fifo32_put(struct FIFO32 *fifo, int data) {//����һ�ֽڵ����ݲ�����
	if(fifo->free == 0) { //����ռ�< 0 ��flags = 1
		fifo->flags |= FLAGS_OVERRUN;
		return -1; //�������-1
	}
	fifo->buf[fifo->p] = data;
	fifo->p++; //pָ����һλ��
	if(fifo->p == fifo->size) { //������ͷ��ʼд
		fifo->p = 0;
	}
	fifo->free--;
	return 0;
}

int fifo32_get(struct FIFO32 *fifo) {//ȡ��һ�ֽڵ�����
	int data;
	if(fifo->free == fifo->size) { //������Ϊ�շ���-1
		return -1;
	}
	data = fifo->buf[fifo->q];
	fifo->q++;
	if(fifo->q == fifo->size) { //������ͷ��ʼ��
		fifo->q = 0;
	}
	fifo->free++;
	return data;
}

int fifo32_status(struct FIFO32 *fifo) { //�鿴�����˶�������
	return fifo->size - fifo->free;
}
