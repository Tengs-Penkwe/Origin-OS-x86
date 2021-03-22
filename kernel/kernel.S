[bits 32]
%define ERROR_CODE	nop
%define ZERO		push 0

extern idt_table
global intr_entry_table
global intr_exit

;----------------------------------------
;Macro for Interrupt Handler Function
;----------------------------------------
	%macro	VECTOR 2
		SECTION .text
		intr%1entry:

			%2						;In case there is error code
			push ds
			push es
			push fs
			push gs
			pushad					;A->C->D->B->SP->BP->SI->DI

			;Send EOI to 8259As to let new Int in
				mov al,0x20
			out 0xa0,al				;slave
			out 0x20,al				;master

			push %1					;Interrupt number
		call [idt_table + %1*4]
		jmp intr_exit

		SECTION .data
		dd intr%1entry

	%endmacro
;----------------------------------------

;----------------------------------------
;Exit Interrupt
;----------------------------------------
SECTION .text
	intr_exit:
		add esp,4	;jump over int number
		popad
		pop gs
		pop fs
		pop es
		pop ds
		add esp,4	;jump over error code
	iretd
;----------------------------------------

SECTION .data
	intr_entry_table:
;----------------------------------------
;Interrupt Table
;----------------------------------------
	VECTOR 0x00,ZERO
	VECTOR 0x01,ZERO
	VECTOR 0x02,ZERO
	VECTOR 0x03,ZERO 
	VECTOR 0x04,ZERO
	VECTOR 0x05,ZERO
	VECTOR 0x06,ZERO
	VECTOR 0x07,ZERO 
	VECTOR 0x08,ERROR_CODE
	VECTOR 0x09,ZERO
	VECTOR 0x0a,ERROR_CODE
	VECTOR 0x0b,ERROR_CODE 
	VECTOR 0x0c,ZERO
	VECTOR 0x0d,ERROR_CODE
	VECTOR 0x0e,ERROR_CODE
	VECTOR 0x0f,ZERO 
	VECTOR 0x10,ZERO
	VECTOR 0x11,ERROR_CODE
	VECTOR 0x12,ZERO
	VECTOR 0x13,ZERO 
	VECTOR 0x14,ZERO
	VECTOR 0x15,ZERO
	VECTOR 0x16,ZERO
	VECTOR 0x17,ZERO 
	VECTOR 0x18,ERROR_CODE
	VECTOR 0x19,ZERO
	VECTOR 0x1a,ERROR_CODE
	VECTOR 0x1b,ERROR_CODE 
	VECTOR 0x1c,ZERO
	VECTOR 0x1d,ERROR_CODE
	VECTOR 0x1e,ERROR_CODE
	VECTOR 0x1f,ZERO 
	VECTOR 0x20,ZERO			;Timer
	VECTOR 0x21,ZERO			;Keyboard
	VECTOR 0x22,ZERO			;Cascade
	VECTOR 0x23,ZERO			;Serial 2
	VECTOR 0x24,ZERO			;Serial 1
	VECTOR 0x25,ZERO			;Parallel 2
	VECTOR 0x26,ZERO			;Floppy Disk
	VECTOR 0x27,ZERO			;Parallel 1
	VECTOR 0x28,ZERO			;Real-timr Clock
	VECTOR 0x29,ZERO			;Redirection
	VECTOR 0x2a,ZERO			;RESERVED
	VECTOR 0x2b,ZERO			;RESERVED
	VECTOR 0x2c,ZERO			;PS/2 Mouse
	VECTOR 0x2d,ZERO			;FPU Float Unit Error
	VECTOR 0x2e,ZERO			;Hard Disk
	VECTOR 0x2f,ZERO			;RESERVED

