; haribote-os boot asm
; TAB=4

[INSTRSET "i486p"]				; 要使用486的指令：LGDT，EAX，CR0等

VBEMODE	EQU		0x105			; 1024 x  768 x 8bitカラー
; （画面モード一覧）
;	0x100 :  640 x  400 x 8bit彩色
;	0x101 :  640 x  480 x 8bit彩色
;	0x103 :  800 x  600 x 8bit彩色
;	0x105 : 1024 x  768 x 8bit彩色
;	0x107 : 1280 x 1024 x 8bit彩色

BOTPAK	EQU		0x00280000		; bootpackのロード先
DSKCAC	EQU		0x00100000		; ディスクキャッシュの場所
DSKCAC0	EQU		0x00008000		; ディスクキャッシュの場所（リアルモード）

; BOOT_INFO
CYLS EQU 0x0ff0
LEDS EQU 0x0ff1 
VMODE EQU 0x0ff2 ; 颜色的位数
SCRNX EQU 0x0ff4 ; 分辨率X
SCRNY EQU 0x0ff6 ; 分辨率Y
VRAM  EQU 0x0ff8 ; 图像缓冲区的开始地址

	ORG 0xc200
	
;确认VBE是否存在，有则ax为0x004f
	MOV AX,0x9000
	MOV ES,AX
	MOV DI,0
	MOV AX,0x4f00
	INT 0x10
	CMP AX,0x004f
	JNE scrn320
	
;检查VBE的版本
	MOV AX,[ES:DI+4]
	CMP AX,0x0200
	JB scrn320		; if (AX < 0x0200) goto scrn320

;取得画面模式信息
	MOV CX,VBEMODE
	MOV AX,0x4f01
	INT 0x10
	CMP AX,0x004f
	JNE scrn320

;画面模式信息的确认
	CMP		BYTE [ES:DI+0x19],8;颜色数是否为8
	JNE		scrn320
	CMP		BYTE [ES:DI+0x1b],4;是否为调色板模式
	JNE		scrn320
	MOV		AX,[ES:DI+0x00]
	AND		AX,0x0080
	JZ		scrn320;属性模式的bit7是0，所以放弃

;画面模式的切换
	MOV		BX,VBEMODE+0x4000
	MOV		AX,0x4f02
	INT		0x10
	MOV		BYTE [VMODE],8	; 记录下画面模式
	MOV		AX,[ES:DI+0x12]
	MOV		[SCRNX],AX	;X的分辨率
	MOV		AX,[ES:DI+0x14]
	MOV		[SCRNY],AX	;Y的分辨率
	MOV		EAX,[ES:DI+0x28]
	MOV		[VRAM],EAX	;VRAM的地址
	JMP		keystatus

;设定320画面模式
scrn320:
	MOV		AL,0x13	;VGA显卡，320*200*8位色彩
	MOV		AH,0x00
	INT 0x10
	MOV BYTE [VMODE],8 ;记录画面模式
	MOV		WORD [SCRNX],320
	MOV		WORD [SCRNY],200
	MOV		DWORD [VRAM],0x000a0000
	
;用BIOS取得键盘指示灯的状态
keystatus:
	MOV AH,0x02
	INT 0x16
	MOV [LEDS],AL

;	禁止主PIC和从PIC的一切中断
;	根据AT兼容机的规格，如果要初始化PIC，必须在CLI之前，否则有时候会挂起
;	随后进行PIC的初始化

		MOV		AL,0xff
		OUT		0x21,AL
		NOP						; 只是让CPU休息一个时钟，如果连续执行OUT，有些机器无法实现
		OUT		0xa1,AL

		CLI						; 禁止CPU级别的中断

;	为了让CPU访问1MB以上的内存空间，设定A20GATE

		CALL	waitkbdout	; 相当于void wait_KBC_sendready(void) 等待键盘控制电路准备完毕
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL
		CALL	waitkbdout	; 为等待完成执行命令

; 切换到保护模式

		LGDT	[GDTR0]			; 设定临时GDT
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; bit31置0，为禁止分页
		OR		EAX,0x00000001	; bit0置1，为切换到保护模式：受保护的虚拟内存地址模式，相对：实际地址模式
		MOV		CR0,EAX	; 实现模式转换
		JMP		pipelineflush	; 通过CR0实现模式改变时，接着要立即使用JMP指令
pipelineflush:
		MOV		AX,1*8			;  0x0008相当于gdt+1
		MOV		DS,AX	; 除了CS外所有的段寄存器的意思都变了
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; bootpack的转送

		MOV		ESI,bootpack	; 转送源
		MOV		EDI,BOTPAK		; 转送目的地
		MOV		ECX,512*1024/4
		CALL	memcpy

; 磁盘数据最终转送到它本来的位置上去

; 首先从启动扇区开始

		MOV		ESI,0x7c00		; 转送源
		MOV		EDI,DSKCAC		; 转送目的地
		MOV		ECX,512/4
		CALL	memcpy

; 剩余的全部，haribotes.sys是asmheadbin和bootpack.hrb连接起来而生成的

		MOV		ESI,DSKCAC0+512	; 转送源
		MOV		EDI,DSKCAC+512	; 转送目的地
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; 从柱面数转换为字节数
		SUB		ECX,512/4		; 减去IPL的512字节
		CALL	memcpy

; 必须由asmhead来完成的工作，至此全部完毕，以后由bootpack完成

; bootpack的启动

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; 没有要转送的东西时
		MOV		ESI,[EBX+20]	; 转送源
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 转送目的地
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; 栈初始值
		JMP		DWORD 2*8:0x0000001b

waitkbdout: ; 相当于void wait_KBC_sendready(void) 等待键盘控制电路准备完毕
		IN		 AL,0x64
		AND		 AL,0x02
		IN		 AL,0x60	; 空读：为了清空数据接受缓冲区的辣鸡数据
		JNZ		waitkbdout
		RET

memcpy:	;相当于memcpy(转送源地址，转送目的地址，转送数据的大小)
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy
		RET
		

		ALIGNB	16	; 一直添加DBO，直到地址能被16整除时，地址不是8的整数倍，向段寄存器复制的MOV指令就会慢一些
GDT0: ;特殊的GDT，0号是空区域，不能定义段
		RESB	8				; ヌルセレクタ
		DW		0xffff,0x0000,0x9200,0x00cf	; 可以读写的段32bit
		DW		0xffff,0x0000,0x9a28,0x0047	; 可以执行的段32bit（bootpack用）

		DW		0
GDTR0: ; 是LGDT指令
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack: