[bits 32]
%define ERROR_CODE	nop
%define ZERO		push 0

extern idt_table
global intr_entry_table
global intr_exit

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

SECTION .data
	intr_entry_table:

SECTION .text
	intr_exit:
		add esp,4	;jump over int number
		popad
		pop gs
		pop es
		pop ds
		add esp,4	;jump over error code
	iretd

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
	VECTOR 0x20,ZERO

