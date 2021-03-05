%include "boot.inc"
;extern _read_n_disk

;主引导程序 
;------------------------------------------------------------
SECTION MBR vstart=0x7c00

	;Copy CS to ES,FS 
	;cs points to 0x7c00(MBR)
		mov ax,cs
	mov es,ax
	mov fs,ax
	
	;Set Stack
	;SS:0x9000 -> SP:0xfbff
	;The Highest Usable Space
		mov ax,0x9000
	mov ss,ax
	mov sp,0xfbff

	;Prepare to Output Character
		mov ax,0xb800		;text mode display
	mov gs,ax

;------------------------------------------------------------
;Clear Screen
;use 0x10 -> 0x06 interrupt
;-----------------------------------------------------------
;INT 0x10   功能号:0x06	   功能描述:上卷窗口
;------------------------------------------------------
;AH 功能号= 0x06
;AL = 上卷的行数(如果为0,表示全部)
;BH = 上卷行属性
;(CL,CH) = 窗口左上角的(X,Y)位置
;(DL,DH) = 窗口右下角的(X,Y)位置
;无返回值：

	mov ax,06_00h
	mov bx,07_00h
	mov cx,00_00
	mov dx,18_4fh		; 0x18 = 25, 0x4f = 80 
					; in VGA text mode, only 80*25 characters
	int 10h
;------------------------------------------------------------

	; 输出字符串:MBR
	mov byte [gs:0x00],'1'
	mov byte [gs:0x01],0xA4

	mov byte [gs:0x02],' '
	mov byte [gs:0x03],0xA4

	mov byte [gs:0x04],'M'
	mov byte [gs:0x05],0xA4	   ;A表示绿色背景闪烁，4表示前景色为红色

	mov byte [gs:0x06],'B'
	mov byte [gs:0x07],0xA4

	mov byte [gs:0x08],'R'
	mov byte [gs:0x09],0xA4
;------------------------------------------------------------
;load loader code from disk to memory
;use _read_n_disk() in func.S
;------------------------------------------------------------
;EAX	= LBA sector
;DS:BX	= Memory Address to Write
;CX		= Sectors to Read
;No Return Value

	mov eax,LOADER_START_SECTOR
		mov bx,0
	mov ds,bx
	mov bx,LOADER_BASE_ADDR
	mov cx,4
	call _read_n_disk

	jmp LOADER_BASE_ADDR

%include "func.S"

;------------------------------------------------------------
	times 	510-($-$$)	db	0
	db	0x55,0xaa
