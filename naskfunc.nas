; naskfunc
; TAB=4

;�û������д�� C�����в�֧�ֵĺ���

[FORMAT "WCOFF"] ; Ŀ���ļ��ĸ�ʽ
[BITS 32] ; 32λ

;Ŀ���ļ�����Ϣ
[FILE "naskfunc.nas"] ; Դ�ļ���
	
	; �����а����ĺ�����
	GLOBAL _io_hlt

;ʵ�ʺ���
[SECTION .text]

_io_hlt:
	HLT
	RET