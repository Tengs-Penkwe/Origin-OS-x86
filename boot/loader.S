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

; -------------------------------------
;detect memory size
; -------------------------------------

	total_mem_bytes	dd	0

	ards_buf times 250	db	0
	ards_nr			dw	0

; -------------------------------------
loader_start:
; -------------------------------------


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
;------------------------------------------------------------
;load kernel
;------------------------------------------------------------
	mov eax, KERNEL_START_SECTOR
	mov ebx, KERNEL_BIN_BASE_ADDR
		mov ecx,20
	call _read_n_disk_32

	mov byte [gs:160],'L'
;------------------------------------------------------------
;Initialize Page Directory and Tables
;------------------------------------------------------------
	call setup_page 

	sgdt [gdt_ptr]

	;video segment
	mov ebx, [gdt_ptr + 2]
	or dword [ebx + 0x18 + 4], 0xc000_0000	 ;third segment, change the segment base address
	;stack
	add esp,0xc000_0000
	;GDT
	add dword [gdt_ptr + 2], 0xc000_0000

	; ---- ---- open CR3 ---- ----
		mov eax, PAGE_DIR_TABLE_POS
	mov cr3,eax
	;---------- open PG of CR0 ----------  
		mov eax,cr0
		or eax,0x8000_0000
	mov cr0,eax

	;reload GDT
	lgdt [gdt_ptr]

;------------------------------------------------------------
;Enter Kernel
;------------------------------------------------------------
	;refresh gdt(not needed)
	jmp SELECTOR_CODE:enter_kernel
enter_kernel:
	mov byte [gs:160],'E'

	call kernel_init
	mov byte [gs:160],'K'
	mov esp, KERNEL_STACK_TOP
	jmp KERNEL_ENTRY_POINT


%include "loader_func.S"
