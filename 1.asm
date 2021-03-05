section .bss
resb 2*32

section file1data

strHello db "Hola Mundo!",0ah
STRLEN	equ $-strHello

section file1text
extern print

global _start

_start:
	push STRLEN
	push strHello
	call print

	mov ebx,0
	mov eax,1
	int 0x80
