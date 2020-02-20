; haribote-os boot asm
; TAB=4

[INSTRSET "i486p"]				; Ҫʹ��486��ָ�LGDT��EAX��CR0��

VBEMODE	EQU		0x105			; 1024 x  768 x 8bit����`
; �������`��һ�E��
;	0x100 :  640 x  400 x 8bit��ɫ
;	0x101 :  640 x  480 x 8bit��ɫ
;	0x103 :  800 x  600 x 8bit��ɫ
;	0x105 : 1024 x  768 x 8bit��ɫ
;	0x107 : 1280 x 1024 x 8bit��ɫ

BOTPAK	EQU		0x00280000		; bootpack�Υ�`����
DSKCAC	EQU		0x00100000		; �ǥ���������å���Έ���
DSKCAC0	EQU		0x00008000		; �ǥ���������å���Έ������ꥢ���`�ɣ�

; BOOT_INFO
CYLS EQU 0x0ff0
LEDS EQU 0x0ff1 
VMODE EQU 0x0ff2 ; ��ɫ��λ��
SCRNX EQU 0x0ff4 ; �ֱ���X
SCRNY EQU 0x0ff6 ; �ֱ���Y
VRAM  EQU 0x0ff8 ; ͼ�񻺳����Ŀ�ʼ��ַ

	ORG 0xc200
	
;ȷ��VBE�Ƿ���ڣ�����axΪ0x004f
	MOV AX,0x9000
	MOV ES,AX
	MOV DI,0
	MOV AX,0x4f00
	INT 0x10
	CMP AX,0x004f
	JNE scrn320
	
;���VBE�İ汾
	MOV AX,[ES:DI+4]
	CMP AX,0x0200
	JB scrn320		; if (AX < 0x0200) goto scrn320

;ȡ�û���ģʽ��Ϣ
	MOV CX,VBEMODE
	MOV AX,0x4f01
	INT 0x10
	CMP AX,0x004f
	JNE scrn320

;����ģʽ��Ϣ��ȷ��
	CMP		BYTE [ES:DI+0x19],8;��ɫ���Ƿ�Ϊ8
	JNE		scrn320
	CMP		BYTE [ES:DI+0x1b],4;�Ƿ�Ϊ��ɫ��ģʽ
	JNE		scrn320
	MOV		AX,[ES:DI+0x00]
	AND		AX,0x0080
	JZ		scrn320;����ģʽ��bit7��0�����Է���

;����ģʽ���л�
	MOV		BX,VBEMODE+0x4000
	MOV		AX,0x4f02
	INT		0x10
	MOV		BYTE [VMODE],8	; ��¼�»���ģʽ
	MOV		AX,[ES:DI+0x12]
	MOV		[SCRNX],AX	;X�ķֱ���
	MOV		AX,[ES:DI+0x14]
	MOV		[SCRNY],AX	;Y�ķֱ���
	MOV		EAX,[ES:DI+0x28]
	MOV		[VRAM],EAX	;VRAM�ĵ�ַ
	JMP		keystatus

;�趨320����ģʽ
scrn320:
	MOV		AL,0x13	;VGA�Կ���320*200*8λɫ��
	MOV		AH,0x00
	INT 0x10
	MOV BYTE [VMODE],8 ;��¼����ģʽ
	MOV		WORD [SCRNX],320
	MOV		WORD [SCRNY],200
	MOV		DWORD [VRAM],0x000a0000
	
;��BIOSȡ�ü���ָʾ�Ƶ�״̬
keystatus:
	MOV AH,0x02
	INT 0x16
	MOV [LEDS],AL

;	��ֹ��PIC�ʹ�PIC��һ���ж�
;	����AT���ݻ��Ĺ�����Ҫ��ʼ��PIC��������CLI֮ǰ��������ʱ������
;	������PIC�ĳ�ʼ��

		MOV		AL,0xff
		OUT		0x21,AL
		NOP						; ֻ����CPU��Ϣһ��ʱ�ӣ��������ִ��OUT����Щ�����޷�ʵ��
		OUT		0xa1,AL

		CLI						; ��ֹCPU������ж�

;	Ϊ����CPU����1MB���ϵ��ڴ�ռ䣬�趨A20GATE

		CALL	waitkbdout	; �൱��void wait_KBC_sendready(void) �ȴ����̿��Ƶ�·׼�����
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL
		CALL	waitkbdout	; Ϊ�ȴ����ִ������

; �л�������ģʽ

		LGDT	[GDTR0]			; �趨��ʱGDT
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; bit31��0��Ϊ��ֹ��ҳ
		OR		EAX,0x00000001	; bit0��1��Ϊ�л�������ģʽ���ܱ����������ڴ��ַģʽ����ԣ�ʵ�ʵ�ַģʽ
		MOV		CR0,EAX	; ʵ��ģʽת��
		JMP		pipelineflush	; ͨ��CR0ʵ��ģʽ�ı�ʱ������Ҫ����ʹ��JMPָ��
pipelineflush:
		MOV		AX,1*8			;  0x0008�൱��gdt+1
		MOV		DS,AX	; ����CS�����еĶμĴ�������˼������
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; bootpack��ת��

		MOV		ESI,bootpack	; ת��Դ
		MOV		EDI,BOTPAK		; ת��Ŀ�ĵ�
		MOV		ECX,512*1024/4
		CALL	memcpy

; ������������ת�͵���������λ����ȥ

; ���ȴ�����������ʼ

		MOV		ESI,0x7c00		; ת��Դ
		MOV		EDI,DSKCAC		; ת��Ŀ�ĵ�
		MOV		ECX,512/4
		CALL	memcpy

; ʣ���ȫ����haribotes.sys��asmheadbin��bootpack.hrb�������������ɵ�

		MOV		ESI,DSKCAC0+512	; ת��Դ
		MOV		EDI,DSKCAC+512	; ת��Ŀ�ĵ�
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; ��������ת��Ϊ�ֽ���
		SUB		ECX,512/4		; ��ȥIPL��512�ֽ�
		CALL	memcpy

; ������asmhead����ɵĹ���������ȫ����ϣ��Ժ���bootpack���

; bootpack������

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; û��Ҫת�͵Ķ���ʱ
		MOV		ESI,[EBX+20]	; ת��Դ
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; ת��Ŀ�ĵ�
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; ջ��ʼֵ
		JMP		DWORD 2*8:0x0000001b

waitkbdout: ; �൱��void wait_KBC_sendready(void) �ȴ����̿��Ƶ�·׼�����
		IN		 AL,0x64
		AND		 AL,0x02
		IN		 AL,0x60	; �ն���Ϊ��������ݽ��ܻ���������������
		JNZ		waitkbdout
		RET

memcpy:	;�൱��memcpy(ת��Դ��ַ��ת��Ŀ�ĵ�ַ��ת�����ݵĴ�С)
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy
		RET
		

		ALIGNB	16	; һֱ���DBO��ֱ����ַ�ܱ�16����ʱ����ַ����8������������μĴ������Ƶ�MOVָ��ͻ���һЩ
GDT0: ;�����GDT��0���ǿ����򣬲��ܶ����
		RESB	8				; �̥륻�쥯��
		DW		0xffff,0x0000,0x9200,0x00cf	; ���Զ�д�Ķ�32bit
		DW		0xffff,0x0000,0x9a28,0x0047	; ����ִ�еĶ�32bit��bootpack�ã�

		DW		0
GDTR0: ; ��LGDTָ��
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack: