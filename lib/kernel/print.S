;Definitions
TI_GDT			equ	0
RPL0			equ	0
SELECTOR_VIDEO	equ	(0x0003<<3) + TI_GDT + RPL0

[bits 32]
section .text
;------------------------------ put_char ------------------------------
global	put_char
;put a charactor in stack to cursor
put_char:
;----------------------------------------------------------------------
;ESP is inside 
		pushad
			mov ax, SELECTOR_VIDEO
		mov gs,ax

;--------------------------------------------------
;get the place of the current cursor
;--------------------------------------------------

	;--------------------
	;Higher 8 bits
	;--------------------
			mov dx,0x3d4
			mov al,0x0e
		out dx,al
			mov dx,0x3d5
		in al,dx
	mov ah,al

	;--------------------
	;Lower 8 bits
	;--------------------
			mov dx,0x3d4
			mov al,0x0f
		out dx,al
			mov dx,0x3d5
		in al,dx
	mov bx,ax				;store the place of cursor to ax
	;--------------------
	;exam the charactor
	;--------------------
		;the char is inside the stack, since we pushad(ed) before esp+36
			mov ecx,[esp+36]
		;decide what the char is
			cmp cl,0x0d
		jz .is_carrage_return
			cmp cl,0x0a
		jz .is_line_feed
			cmp cl,0x08
		jz .is_backspace

		;If it's not the three ones above, treat it as normal charactor
		jmp	.put_other

;----------------------------------------------------------------------

;--------------------------------------------------
.is_backspace:
;--------------------------------------------------
			dec bx
			shl bx,1

		mov byte [gs:bx], 0x20
		inc bx
		mov byte [gs:bx], 0x07
		
			shr bx,1
		jmp .set_cursor

;--------------------------------------------------
.put_other:
;--------------------------------------------------
			shl bx,1
		mov [gs:bx],cl
		inc bx
		mov [gs:bx],0x07
			shr bx,1

		;change new line if the chars is over 2000
			inc bx
			cmp bx,2000
		jl .set_cursor

;--------------------------------------------------
.is_line_feed:
.is_carrage_return:
;--------------------------------------------------
	
		;------------------------------
		;calculate the place of next line
		;------------------------------

				xor dx,dx 			;higher 16 bits of devident
			mov ax,bx
				mov si,80
			div si

			;set bx to the begin of the line
			sub bx,dx

		.is_carrage_return:
			add bx,80
			cmp bx,2000
		.is_line_feed:
			jl .set_cursor


		








































