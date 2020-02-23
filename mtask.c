#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TASK *task_now(void) {
	//返回现在活动中的task的地址
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	return tl->tasks[tl->now];
}

void task_add(struct TASK *task) {//向某个level添加一个任务
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	tl->tasks[tl->running] = task;
	tl->running++;
	task->flags = 2;//活动中
	return;
}

void task_remove(struct TASK *task) {
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	
	//寻找task所在的位置
	for(i = 0; i < tl->running; i++) {
		if(tl->tasks[i] == task) {
			break;
		}
	}
	
	tl->running--;
	if(i < tl->now) {//不是正在运行的task，需要相应的移动
		tl->now--;
	}
	
	if(tl->now >= tl->running) {//自己remove自己，把当前正在运行设置为0
		tl->now = 0;
	}
	
	task->flags = 1;//休眠中
	for(; i < tl->running; i++) {//移动成员
		tl->tasks[i] = tl->tasks[i+1];
	}
	return;
}

void task_switchsub(void) {
	//任务切换时决定切换到哪个level	
	int i;
	//寻找最上层的level
	for(i = 0; i <MAX_TASKLEVELS; i++) {
		if(taskctl->level[i].running > 0) {
			break;
		}
	}
	taskctl->now_lv = i;
	taskctl->lv_change = 0;
	return;
}

struct TASK *task_init(struct MEMMAN *memman) {
	//使调用init的程序成为一个task
	int i;
	struct TASK *task;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof(struct TASKCTL));
	for(i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].flags = 0;
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;//GDT的编号
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
	}
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		taskctl->level[i].running = 0;
		taskctl->level[i].now = 0;
	}
	task = task_alloc();
	task->flags = 2;//活动中标志
	task->priority = 2; /* 0.02秒 */
	task->level = 0;
	task_add(task);
	task_switchsub();//taskctl->nowlv设置
	load_tr(task->sel);
	task_timer = timer_alloc();
	timer_settime(task_timer, task->priority);
	return task;
}

struct TASK *task_alloc(void)
{
	int i;
	struct TASK *task;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1; //使用中
			task->tss.eflags = 0x00000202; /* IF = 1; */
			task->tss.eax = 0;//先设置为0
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;
			task->tss.ldtr = 0;
			task->tss.iomap = 0x40000000;
			return task;
		}
	}
	return 0; /*全部正在使用 */
}

void task_run(struct TASK *task,int level, int priority) {
	if(level < 0) {//不改变level
		level = task->level;
	}
	if(priority > 0) {
		task->priority = priority;
	}
	if(task->flags == 2 && task->level != level) {//要改变活动中的level
		task_remove(task);//这里执行后flags会变成1，所以下个条件也会满足执行
	}
	if(task->flags != 2) {
		//从休眠中唤醒
		task->level = level;
		task_add(task);
	}
	taskctl->lv_change = 1;//下次任务切换时检查level
	return;
}

void task_switch(void) {
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	struct TASK *new_task, *now_task = tl->tasks[tl->now];
	tl->now++;
	if (tl->now == tl->running) {//是最后则从列表头重新开始循环
		tl->now = 0;
	}
	if (taskctl->lv_change != 0) {//需要更改level
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}
	new_task = tl->tasks[tl->now];
	timer_settime(task_timer, new_task->priority);
	if (new_task != now_task) {//不是一个任务更改自己的优先值的情况
		farjmp(0, new_task->sel);
	}
	return;
}

void task_sleep(struct TASK *task) {
	struct TASK *now_task;
	if(task->flags == 2) {
		//如果处于活动状态
		now_task = task_now();
		task_remove(task);//flags会变成1
		if(task == now_task) {
			//如果是让自己休眠，则需要进行任务切换
			task_switchsub();
			now_task = task_now();//设定后获取当前任务的值
			farjmp(0, now_task->sel);
		}
	}
	return;
}
