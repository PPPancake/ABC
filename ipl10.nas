; haribote-ipl
; TAB=4

; ����18��������10������


CYLS EQU 10 ;CYLS=10 10������

	ORG 0x7c00 ; ����Ļ�������װ�ص�0x7c00

; ��׼FAT12��ʽ����ר�õĴ���
	JMP entry
	DB 0x90
	DB "HARIBOTE" ; ���������� 8B
	DW 512 ; ÿ��������С *512B
	DB 1 ; �ش�С *1������
	DW 1 ; FAT��ʼλ�� һ���ǵ�1������
	DB 2 ; FAT�ĸ��� *2
	DW 224 ; ��Ŀ¼��С һ��224��
	DW 2880 ; ���̴�С *2280����
	DB 0xf0 ; �������� *0xf0
	DW 9 ; FAT���� *9����
	DW 18 ; һ���ŵ���*18������
	DW 2 ; ��ͷ��*2
	DD 0 ; ��ʹ�÷���*0
	DD 2880 ; ��д���̴�С
	DB 0,0,0x29 ; *
	DD 0xffffffff ; maybe������
	DB "HARIBOTEOS " ; �������� 11B
	DB "FAT12   " ; ���̸�ʽ���� 8B
	RESB 18 ; �ճ�18B

; ��������
entry:
	MOV AX,0 ;��ʼ���Ĵ���
	MOV SS,AX ;ջ�μĴ���
	MOV SP,0x7c00 ;ջָ��Ĵ���
	MOV DS,AX ;����μĴ��� Ĭ����DS
	
	MOV AX,0x0820
	MOV ES,AX
	MOV CH,0 ; ����0
	MOV DH,0 ; ��ͷ0
	MOV CL,2 ; ����2

readloop:
	MOV SI,0 ; ��¼ʧ�ܴ����ļĴ���
	
retry:
	MOV AH,0x02 ; ���̲���
	MOV AL,1 ; ��һ������
	MOV BX,0 ; ES:BX�����ַ
	MOV DL,0x00 ; 0��������
	INT 0x13 ; ����BIOS
	JNC next ; CF=0 û�д���

	;5���Դ�
	ADD SI,1
	CMP SI,5
	JAE error
	MOV AH,0x00
	MOV DL,0x00
	INT 0x13 ;ϵͳ��λ
	JMP retry

next:
    ; ��18������(CL:1-18)
	MOV AX,ES
	ADD AX,0x0020
	MOV ES,AX ;��ת����һ������
	ADD CL,1
	CMP CL,18
	JBE readloop
	
	;2����ͷ(DH:0-1)
	MOV CL,1
	ADD DH,1
	CMP DH,2
	JB readloop
	
	;$CYLS������(CH:0-79)
	MOV DH,0
	ADD CH,1
	CMP CH,CYLS
	JB readloop
	
	MOV [0x0ff0],CH ;��CYLS��ֵд���ڴ��ַ0x0ff0��
	JMP 0xc200 ;������̺���ת��sys���ݵ�λ��

error:
	MOV SI,msg ;Դ��ַ�Ĵ��� SIָ��msg�ĵ�ַ

putloop:
	MOV AL,[SI] ; MOV AL,[msg��ַ] = msg��1B���ݷ���AL��
	ADD SI,1
	CMP AL,0 ;SI����ֱָ����0��
	JE fin
	MOV AH,0x0e ;AL=character code
	MOV BX,15 ;BH=0,BL=color code
	INT 0x10
	JMP putloop
	
fin:
	HLT ;CPU�������״̬
	JMP fin

msg:
	DB 0x0a, 0x0a ; ��������
	DB "load error"
	DB 0x0a ; ����
	DB 0 
	
	RESB 0x7dfe-$ ; ��0x7dfe֮ǰд0 $:��һ�����ڵ��ֽ���
	
	DB 0x55, 0xaa
