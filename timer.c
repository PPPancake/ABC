#include "bootpack.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

struct TIMERCTL timerctl;

#define TIMER_FLAGS_ALLOC 1
#define TIMER_FLAGS_USING 2

void init_pit(void) {
	int i;
	//实际中断产生频率 = 时钟周期数/设定数值
	//时钟周期数1193180
	//设定数值0x9c2e=11932
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	timerctl.next = 0xffffffff;//因为最近没有正在运行的计时器
	timerctl.using = 0;
	for(i = 0; i < MAX_TIMER; i++) {
		timerctl.timers0[i].flags = 0;
	}
	return;
}

struct TIMER *timer_alloc(void) {
	int i;
	for(i = 0; i < MAX_TIMER; i++) {
		if(timerctl.timers0[i].flags == 0) {
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers0[i];
		}
	}
	return 0;//没找到
}

void timer_free(struct TIMER *timer) {
	timer->flags = 0;//未使用
	return;
}

void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data) {
	timer->fifo = fifo;
	timer->data = data;
	return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
	int e, i, j;
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USING;
	e = io_load_eflags();
	io_cli();
	for(i = 0; i <timerctl.using; i++) {
		if(timerctl.timers[i]->timeout >= timer->timeout){
			break;
		}
	}
	for(j = timerctl.using; j>i; j--) {
		timerctl.timers[j] = timerctl.timers[j - 1];
	}
	timerctl.using++;
	timerctl.timers[i] = timer;
	timerctl.next = timerctl.timers[0]->timeout;
	io_store_eflags(e);
	return;
}


void inthandler20(int *esp) {
	int i, j;
	io_out8(PIC0_OCW2, 0x60);
	//把IRQ——00信号接受完的信息通知给PIC
	timerctl.count++;
	if(timerctl.next > timerctl.count) {//下一个计时还没到，所以结束
		return;
	}
	for(i = 0; i < timerctl.using; i++) {//正在使用的timer中
		//timers的定时器都在使用，所以不确认flags
		if(timerctl.timers[i]->timeout > timerctl.count) {//未超时
			break;
		}
		//超时
		timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
		//到时就向缓冲区放8位的数据，这时缓冲区free=0
		fifo8_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
	}
	//正好有i个计时器超时了，其余的进行移位
	timerctl.using -= i;
	for (j = 0; j < timerctl.using; j++) {
		timerctl.timers[j] = timerctl.timers[j + i];
	}
	//设定新的next
	if (timerctl.using > 0) {
		timerctl.next = timerctl.timers[0]->timeout;
	} else {
		timerctl.next = 0xffffffff;
	}
	return;
}
