#include "bootpack.h"

#define SHEET_USE 1

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize)
{
	struct SHTCTL *ctl;
	int i;
	ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof (struct SHTCTL));
	if (ctl == 0) {
		goto err;
	}
	ctl->map = (unsigned char *) memman_alloc_4k(memman, xsize * ysize);
	if(ctl->map == 0) {
		memman_free_4k(memman, (int) ctl, sizeof(struct SHTCTL));
		goto err;
	}
	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1; //没有一张sheet
	for (i = 0; i < MAX_SHEETS; i++) {
		ctl->sheets0[i].flags = 0;//未使用标记
		ctl->sheets0[i].ctl = ctl;//记录所属
	}
err:
	return ctl;
}
	
struct SHEET *sheet_alloc(struct SHTCTL *ctl) {
	struct SHEET *sht;
	int i;
	for(i = 0; i < MAX_SHEETS; i++) {
		if(ctl->sheets0[i].flags == 0) {
			sht = &ctl->sheets0[i];
			sht->flags = SHEET_USE;//标记为正在使用
			sht->height = -1;//隐藏
			return sht;
		}
	}
	return 0;//所有的sheet都处于使用状态
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv) {
	//设定图层的缓冲区大小和透明色的指数
	sht->buf = buf;
	sht->bxsize = xsize;
	sht->bysize = ysize;
	sht->col_inv = col_inv;
	return;
}

void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, sid, *map = ctl->map;
	struct SHEET *sht;
	//如果refresh的范围超过了画面则修正
	if(vx0 < 0) { vx0 = 0; }
	if(vy0 < 0) { vy0 = 0; }
	if(vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if(vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for(h = h0; h <= ctl->top; h++) {
		sht = ctl->sheets[h];
		sid = sht - ctl->sheets0;//图层号码
		buf = sht->buf;
		bx0 = vx0 - sht->vx0;//vx0 = sht->vx0 + bx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) {bx0 = 0;}
		if (by0 < 0) {by0 = 0;}
		if (bx1 > sht->bxsize) {bx1 = sht->bxsize;}
		if (by1 > sht->bysize) {by1 = sht->bysize;}
		for(by = 0; by < by1; by++) {
			vy = sht->vy0 + by;
			for(bx = 0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				if(buf[by * sht->bxsize + bx] != sht->col_inv) {//不是透明的则送进VRAM
					map[vy * ctl->xsize + vx] = sid;
				}
			}
		}
	}
	return;
}

void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1) {
	//vx相对于屏幕本身的坐标，bx相对于sheet底板的位置
	//将透明以外的所有像素都复制到VRAM中
	//是整个sheet在移动
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, *vram = ctl->vram, *map = ctl->map, sid;
	struct SHEET *sht;
	//如果refresh的范围超过了画面则修正
	if(vx0 < 0) { vx0 = 0; }
	if(vy0 < 0) { vy0 = 0; }
	if(vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if(vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for(h = h0; h <= h1; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;
		sid = sht - ctl->sheets0;
		bx0 = vx0 - sht->vx0;//vx0 = sht->vx0 + bx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) {bx0 = 0;}
		if (by0 < 0) {by0 = 0;}
		if (bx1 > sht->bxsize) {bx1 = sht->bxsize;}
		if (by1 > sht->bysize) {by1 = sht->bysize;}
		for(by = 0; by < by1; by++) {
			vy = sht->vy0 + by;
			for(bx = 0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				if(map[vy * ctl->xsize + vx] == sid) {//不是透明的则送进VRAM
					vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
				}
			}
		}
	}
	return;
}

void sheet_updown(struct SHEET *sht, int height)  {
	//上下移动图层
	
	struct SHTCTL *ctl = sht->ctl;
	int h, old = sht->height;// 记录设置前的高度信息
	
	//如果指定的高度过高或过低，则进行矫正
	if (height > ctl->top + 1) {
		height = ctl->top + 1;
	}
	if (height < -1) {
		height = -1;
	}
	sht->height = height;
	
	//sheets的重新排列
	if(old > height) {//新指定的图层高度小于原来
		if (height >= 0) { //中间的往上提
			for(h = old; h > height; h--) {
				ctl->sheets[h] = ctl->sheets[h-1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
			sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
			sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);
		} else {//如果图层要移动到隐藏
			if(ctl->top > old) {//原来不是最高的图层时，上面的降下来
				for(h = old; h < ctl->top; h++) {
					ctl->sheets[h] = ctl->sheets[h+1];
					ctl->sheets[h]->height = h;
				}
			}
			ctl->top--;
		}
		sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
		sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1);
	} else if (old < height) {//新指定的图层高度大于原来
		if(old >= 0) {//原来不是隐藏状态
			for (h = old; h < height; h++) {//中间的拉下去
				ctl->sheets[h] = ctl->sheets[h + 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
		} else {//原来是隐藏状态
			for (h = ctl->top; h >= height; h--) {//上面的提上去
				ctl->sheets[h + 1] = ctl->sheets[h];
				ctl->sheets[h + 1]->height = h + 1;
			}
			ctl->sheets[height] = sht;
			ctl->top++;
		}
		sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
		sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
	}
	return;
}

void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1) {
	if(sht->height >= 0) {
		sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
	}
	return;
}

void sheet_slide(struct SHEET *sht, int vx0, int vy0) {
	//只是上下左右移动
	int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	if(sht->height >= 0) {
		sheet_refreshmap(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
		sheet_refreshmap(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
		sheet_refreshsub(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height-1);
		sheet_refreshsub(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
	}
	return;
}

void sheet_free(struct SHEET *sht) {
	if(sht->height >= 0) {//如果处于显示状态，则先设定为隐藏
		sheet_updown(sht, -1);
	}
	sht->flags = 0;//未使用标志
	return;
}
