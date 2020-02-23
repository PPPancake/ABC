#include "bootpack.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

struct TIMERCTL timerctl;

#define TIMER_FLAGS_ALLOC 1
#define TIMER_FLAGS_USING 2

void init_pit(void) {
	int i;
	struct TIMER *t;
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
	t = timer_alloc();
	t->timeout = 0xffffffff;
	t->flags = TIMER_FLAGS_USING;
	t->next = 0;
	timerctl.t0 = t;
	timerctl.next = 0xffffffff;
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

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data) {
	timer->fifo = fifo;
	timer->data = data;
	return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
	int e;
	struct TIMER *t, *s;
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USING;
	e = io_load_eflags();
	io_cli();
	t = timerctl.t0;
	if(timer->timeout <= t->timeout) {//新设定的定时器早于定时器队列的第一个
		timerctl.t0 = timer;
		timer->next = t;
		timerctl.next = timer->timeout;
		io_store_eflags(e);
		return;
	}
	for(;;){
		s = t;
		t = t->next;
		if(t == 0) {//错误情况
			break;
		}
		if(timer->timeout <= t->timeout) {//找到符合s timer t的位置时
			s->next = timer;
			timer->next = t;
			io_store_eflags(e);
			return;
		}
	}
}


void inthandler20(int *esp) {
	struct TIMER *timer;
	char ts = 0;
	io_out8(PIC0_OCW2, 0x60);
	//把IRQ——00信号接受完的信息通知给PIC
	timerctl.count++;
	if(timerctl.next > timerctl.count) {//下一个计时还没到，所以结束
		return;
	}
	timer = timerctl.t0;//把最前面的地址赋值给timer
	for(;;) {//正在使用的timer中
		//timers的定时器都在使用，所以不确认flags
		if(timer->timeout > timerctl.count) {//未超时
			break;
		}
		//超时
		timer->flags = TIMER_FLAGS_ALLOC;
		if (timer != task_timer) {
			//到时就向缓冲区放8位的数据，这时缓冲区free=0
			//到时后去掉线性表中的第一个timer
			fifo32_put(timer->fifo, timer->data);
		} else {
			ts = 1; /* mt_timer超时 */
		}
		timer = timer->next;
	}
	timerctl.t0 = timer;
	//设定新的next
	timerctl.next = timer->timeout;
	if (ts != 0) {
		task_switch();
	}
	return;
}
