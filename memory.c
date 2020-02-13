#include "bootpack.h"

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000

unsigned int memtest(unsigned int start, unsigned int end) {
	char flg486 = 0;
	unsigned int eflg, cr0, i;
	
	//CPU是486，则第18位是AC标志位，是386，则18位一直是0
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT;
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if((eflg & EFLAGS_AC_BIT) != 0) {//是486
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT;//设置eflags第18位为0
	io_store_eflags(eflg);
	
	if(flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE;//对CR0操作禁止缓存
		store_cr0(cr0);
	}
	
	i = memtest_sub(start, end);
	
	if(flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;//允许缓存
		store_cr0(cr0);
	}
	
	return i;
}

void memman_init(struct MEMMAN *man) {
	man->frees = 0; // 可用信息数目
	man->maxfrees = 0; //用于观察可用状况：frees的最大值
	man->lostsize = 0; //释放失败的内存大小的总和
	man->losts = 0; //释放失败次数
	return;
}

unsigned int memman_total(struct MEMMAN *man) {//报告剩余内存大小的综合
	unsigned int i, t = 0;
	for(i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size) {
	//分配内存
	unsigned int i, a;
	for(i = 0; i < man->frees; i++) {
		if(man->free[i].size >= size) {//找到了足够大的内存
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if(man->free[i].size == 0) {
				//如果free[i]变成了0，就剪掉一条可用信息
				man->frees--;
				for(; i < man->frees; i++) {
					man->free[i] = man->free[i+1];
				}
			}
			return a;
		}
	}
	return 0;
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size) {
	//释放
	int i, j;
	//为便于归纳内存，先将free[]按照addr的顺序排列（本来free[]是按顺序的）
	for(i = 0; i < man->frees; i++) {
		if(man->free[i].addr > addr) {
			break;
		}
	}
	//free[i-1].addr<addr<free[i].addr
	if(i > 0) {//前面有可用内存
		if(man->free[i-1].addr + man->free[i-1].size == addr) {
			//如果可以和前面连起来
			man->free[i-1].size += size;
			if(i < man->frees) {//后面也有可用内存
				if(addr + size == man->free[i].addr) {//与后面相连
					man->free[i-1].size += man->free[i].size;
					man->frees--;
					for(; i < man->frees; i++) {
						man->free[i] = man->free[i+1];
					}
				}
			}
			return 0;
		}
	}
	if(i < man->frees) {//后面有内存
		if(addr+size == man->free[i].addr) {//与后面相连
			man->free[i].addr = addr;
			man->free[i].size +=size;
			return 0;
		}
	}
	if(man->frees < MEMMAN_FREES) {//前后都不能归纳到一起
		for(j = man->frees; j > i; j--) {
			//free[i]之后的向后移动，腾出点可用空间
			man->free[j] = man->free[j-1];
		}
		man->frees++;
		if(man->maxfrees < man->frees) {
			man->maxfrees = man->frees;//更新最大值
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0;
	}
	//失败
	man->losts++;
	man->lostsize += size;
	return -1;
}

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size) {
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;//向上舍入
	a = memman_alloc(man,size);
	return a;
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size) {
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}
