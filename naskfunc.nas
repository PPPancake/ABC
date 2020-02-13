; naskfunc
; TAB=4

;用汇编语言写的 C语言中不支持的函数

[FORMAT "WCOFF"] ; 目标文件的格式
[INSTRSET "i486p"]
[BITS 32] ; 32位

;目标文件的信息
[FILE "naskfunc.nas"] ; 源文件名
	
	; 程序中包含的函数名
	GLOBAL _io_hlt, _io_cli,_io_sti,_io_stihlt
	GLOBAL _io_in8,_io_in16,_io_in32
	GLOBAL _io_out8,_io_out16,_io_out32
	GLOBAL _io_load_eflags,_io_store_eflags
	GLOBAL _load_gdtr, _load_idtr
	GLOBAL _load_cr0, _store_cr0
	GLOBAL _asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
	GLOBAL _memtest_sub;
	EXTERN	_inthandler21, _inthandler27, _inthandler2c

;实际函数
[SECTION .text]

_io_hlt:	; void io_hlt(void) 让CPU处于睡眠状态
	HLT
	RET

_io_cli:	; void io_cli(void) 禁止中断发生
	CLI
	RET

_io_sti:	; void io_sti(void) 允许中断发生
	STI
	RET

_io_stihlt: ; void io_stihlt(void);
	STI
	HLT
	RET

_io_in8:	; int io_in8(int port) 从端口DX读1字节进入AL
	MOV EDX,[ESP+4]	;port
	MOV EAX,0
	IN AL,DX	;从端口DX读1字节进入AL
	RET

_io_in16:	; int io_in16(int port);
	MOV EDX,[ESP+4]	;port
	MOV EAX,0
	IN AX,DX
	RET

_io_in32:	; int io_in32(int port);
	MOV EDX,[ESP+4]	;port
	IN EAX,DX
	RET

_io_out8:	; void io_out8(int port, int data) 将AL的1字节内容输出至端口DX
	MOV EDX,[ESP+4]	;port
	MOV AL,[ESP+8]	;data
	OUT DX,AL	;将AL的1字节内容输出至端口DX
	RET

_io_out16:	; void io_out16(int port, int data);
	MOV EDX,[ESP+4]	;port
	MOV EAX,[ESP+8]	;data
	OUT DX,AX
	RET

_io_out32:	; void io_out32(int port, int data);
	MOV EDX,[ESP+4]	;port
	MOV EAX,[ESP+8]	;data
	OUT DX,EAX
	RET

_io_load_eflags:	; int io_load_eflags(void) 取出EFLAGS
	PUSHFD
	POP EAX
	RET	;C语言中EAX的值被看作返回值

_io_store_eflags:	; void io_store_eflags(int eflags) 存入EFLAGS
	MOV EAX,[ESP+4]
	PUSH EAX
	POPFD
	RET

_load_gdtr:	; void load_gdtr(int limit, int addr) 将指定的段上限和地址赋值给GDTR这一48位寄存器
	MOV AX,[ESP+4]
	MOV [ESP+6],AX
	LGDT [ESP+6];不能用MOV，唯一指令
	RET

_load_idtr:	; void load_idtr(int limit, int addr)  将指定的段上限和地址赋值给GDTR这一48位寄存器
	MOV AX,[ESP+4]
	MOV [ESP+6],AX
	LIDT [ESP+6];不能用MOV，唯一指令
	RET
	
_load_cr0:	; int load_cr0(void);
	MOV EAX,CR0
	RET

_store_cr0:	; void store_cr0(int cr0);
	MOV EAX,[ESP+4]
	MOV CR0,EAX
	RET

	
_asm_inthandler21: ;中断处理完成后，必须使用IRETD指令
	PUSH	ES ;将寄存器的值保存到栈里，另DS和ES = SS
	PUSH	DS
	PUSHAD
	MOV		EAX,ESP
	PUSH	EAX
	MOV		AX,SS
	MOV		DS,AX
	MOV		ES,AX
	CALL	_inthandler21 ;执行inthandler21指令
	POP		EAX ;返回原来保存的值
	POPAD
	POP		DS
	POP		ES
	IRETD

_asm_inthandler27:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler27
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler2c:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler2c
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_memtest_sub: ; unsigned int memtest_sub(unsigned int start, unsigned int end)
	PUSH EDI	;还要使用EDI，ESI，EBX
	PUSH ESI
	PUSH EBX
	MOV ESI,0xaa55aa55			; pat0 = 0xaa55aa55;
	MOV EDI,0x55aa55aa			; pat1 = 0x55aa55aa;
	MOV EAX,[ESP+12+4]			; i = start;
mts_loop:
	MOV EBX,EAX
	ADD EBX,0xffc				; p = i + 0xffc;
	MOV EDX,[EBX]				; old = *p;
	MOV [EBX],ESI				; *p = pat0;
	XOR DWORD [EBX],0xffffffff	; *p ^= 0xffffffff;
	CMP EDI,[EBX]				; if (*p != pat1) goto fin;
	JNE mts_fin
	XOR DWORD [EBX],0xffffffff	; *p ^= 0xffffffff;
	CMP ESI,[EBX]				; if (*p != pat0) goto fin;
	JNE mts_fin
	MOV [EBX],EDX				; *p = old;
	ADD EAX,0x1000				; i += 0x1000;
	CMP EAX,[ESP+12+8]			; if (i <= end) goto mts_loop;
	JBE mts_loop
	POP EBX
	POP ESI
	POP EDI
	RET
mts_fin:
	MOV [EBX],EDX				; *p = old;
	POP EBX
	POP ESI
	POP EDI
	RET
