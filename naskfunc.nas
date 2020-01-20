; naskfunc
; TAB=4

;用汇编语言写的 C语言中不支持的函数

[FORMAT "WCOFF"] ; 目标文件的格式
[BITS 32] ; 32位

;目标文件的信息
[FILE "naskfunc.nas"] ; 源文件名
	
	; 程序中包含的函数名
	GLOBAL _io_hlt

;实际函数
[SECTION .text]

_io_hlt:
	HLT
	RET