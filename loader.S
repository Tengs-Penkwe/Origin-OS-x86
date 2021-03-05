%include "boot.inc"

SECTION loader vstart=LOADER_BASE_ADDR

	jmp loader_start

; -------------------------------------
;set global descriptor
; ------------------------------------
	
	GDT_BASE:		dd	0x0000_0000
					dd	0x0000_0000

	CODE_DESC:		dd	0x0000_FFFF
					dd	DESC_CODE_HIGH4

   DATA_STACK_DESC:	dd	0x0000_FFFF
					dd	DESC_DATA_HIGH4

	VIDDEO_DESC:	dd	0x8000_0007			;limit=0xbfff-0xb800/4k = 0x7
					dd	DESC_VIDEO_HIGH4

	GDT_SIZE		equ	$-GDT_BASE
	GDT_LIMIT		equ	GDT_SIZE-1

	times 60		dq	0

; -------------------------------------
;set segment selector
; -------------------------------------

	SELECTOR_CODE	equ	(0x0001<<3) + TI_GDT + RPL0
	SELECTOR_DATA	equ	(0x0002<<3) + TI_GDT + RPL0
	SELECTOR_VIDEO	equ	(0x0003<<3) + TI_GDT + RPL0

	;Pointer to GDT 
	gdt_ptr			dw	GDT_LIMIT
					dd	GDT_BASE

	loader_msg		db	'2 loader in real.'

; -------------------------------------
loader_start:
; -------------------------------------

  ; mov	 sp, LOADER_BASE_ADDR

;------------------------------------------------------------
;INT 0x10    功能号:0x13    功能描述:打印字符串
;------------------------------------------------------------
;AH 子功能号=13H
;BH = 页码
;BL = 属性(若AL=00H或01H)
;CX＝字符串长度
;(DH、DL)＝坐标(行、列)
;ES:BP＝字符串地址 
;AL＝显示输出方式
;   0——字符串中只含显示字符，其显示属性在BL中。显示后，光标位置不变
;   1——字符串中只含显示字符，其显示属性在BL中。显示后，光标位置改变
;   2——字符串中含显示字符和显示属性。显示后，光标位置不变
;   3——字符串中含显示字符和显示属性。显示后，光标位置改变
;无返回值
 	mov	 bp, loader_msg   ; ES:BP = 字符串地址
	mov	 cx, 17			 ; CX = 字符串长度
	mov	 ax, 0x1301		 ; AH = 13,  AL = 01h
	mov	 bx, 0x001f		 ; 页号为0(BH = 0) 蓝底粉红字(BL = 1fh)
	mov	 dx, 0x1800		 ;
	int	 0x10            ; 10h 号中断
;------------------------------------------------------------


;------------------------------------------------------------
;Prepare to enter Protected Mode
;------------------------------------------------------------

	;Step 1: Open A20
	in al,0x92
	or al,0000_0010B
	out 0x92,al

	;Step 2: Load GDT
	lgdt	[gdt_ptr]

	;Setp 3: Set CR0 Register
	mov eax,cr0
	or eax,0x0000_0001
	mov cr0,eax

	;Refresh the Pipeline
	jmp dword SELECTOR_CODE:p_mode_start
	;jmp SELECTOR_CODE:p_mode_start

;------------------------------------------------------------
;Inside Protected Mode
;------------------------------------------------------------
[bits 32]
p_mode_start:
	;load data segment descriptor
		mov ax,SELECTOR_DATA
	mov ds,ax
	mov es,ax
	mov ss,ax

	mov esp,LOADER_STACK_TOP

		mov ax,SELECTOR_VIDEO
	mov gs,ax

	mov byte [gs:160],'P'

	jmp $

;------------------------------------------------------------
