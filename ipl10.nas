; haribote-ipl
; TAB=4

; 读入18个扇区，10个柱面


CYLS EQU 10 ;CYLS=10 10个柱面

	ORG 0x7c00 ; 代码的机器语言装载到0x7c00

; 标准FAT12格式软盘专用的代码
	JMP entry
	DB 0x90
	DB "HARIBOTE" ; 启动区名称 8B
	DW 512 ; 每个扇区大小 *512B
	DB 1 ; 簇大小 *1个扇区
	DW 1 ; FAT起始位置 一般是第1个扇区
	DB 2 ; FAT的个数 *2
	DW 224 ; 根目录大小 一般224项
	DW 2880 ; 磁盘大小 *2280扇区
	DB 0xf0 ; 磁盘种类 *0xf0
	DW 9 ; FAT长度 *9扇区
	DW 18 ; 一个磁道有*18个扇区
	DW 2 ; 磁头数*2
	DD 0 ; 不使用分区*0
	DD 2880 ; 重写磁盘大小
	DB 0,0,0x29 ; *
	DD 0xffffffff ; maybe卷标号码
	DB "HARIBOTEOS " ; 磁盘名称 11B
	DB "FAT12   " ; 磁盘格式名称 8B
	RESB 18 ; 空出18B

; 程序主体
entry:
	MOV AX,0 ;初始化寄存器
	MOV SS,AX ;栈段寄存器
	MOV SP,0x7c00 ;栈指针寄存器
	MOV DS,AX ;代码段寄存器 默认是DS
	
	MOV AX,0x0820
	MOV ES,AX
	MOV CH,0 ; 柱面0
	MOV DH,0 ; 磁头0
	MOV CL,2 ; 扇区2

readloop:
	MOV SI,0 ; 记录失败次数的寄存器
	
retry:
	MOV AH,0x02 ; 读盘操作
	MOV AL,1 ; 读一个扇区
	MOV BX,0 ; ES:BX缓冲地址
	MOV DL,0x00 ; 0号驱动器
	INT 0x13 ; 调用BIOS
	JNC next ; CF=0 没有错误

	;5次试错
	ADD SI,1
	CMP SI,5
	JAE error
	MOV AH,0x00
	MOV DL,0x00
	INT 0x13 ;系统复位
	JMP retry

next:
    ; 读18个扇区(CL:1-18)
	MOV AX,ES
	ADD AX,0x0020
	MOV ES,AX ;跳转到下一个扇区
	ADD CL,1
	CMP CL,18
	JBE readloop
	
	;2个磁头(DH:0-1)
	MOV CL,1
	ADD DH,1
	CMP DH,2
	JB readloop
	
	;$CYLS个柱面(CH:0-79)
	MOV DH,0
	ADD CH,1
	CMP CH,CYLS
	JB readloop
	
	MOV [0x0ff0],CH ;将CYLS的值写到内存地址0x0ff0中
	JMP 0xc200 ;读完磁盘后跳转到sys内容的位置

error:
	MOV SI,msg ;源变址寄存器 SI指向msg的地址

putloop:
	MOV AL,[SI] ; MOV AL,[msg地址] = msg的1B内容放在AL中
	ADD SI,1
	CMP AL,0 ;SI向下指直到“0”
	JE fin
	MOV AH,0x0e ;AL=character code
	MOV BX,15 ;BH=0,BL=color code
	INT 0x10
	JMP putloop
	
fin:
	HLT ;CPU进入待机状态
	JMP fin

msg:
	DB 0x0a, 0x0a ; 两个换行
	DB "load error"
	DB 0x0a ; 换行
	DB 0 
	
	RESB 0x7dfe-$ ; 在0x7dfe之前写0 $:这一行现在的字节数
	
	DB 0x55, 0xaa
