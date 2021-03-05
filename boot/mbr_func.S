global _read_n_disk_16

;------------------------------------------------------------
;Read n Sectors in Hard Disk
_read_n_disk_16:
;------------------------------------------------------------
;EAX	= LBA sector
;DS:BX	= Memory Address to Write
;CX		= Sectors to Read
;No Return Value

	push cx

	;Step 1: set parameter: sectors to read
	push eax
		;port 0x1f2(sector count)
		mov dx,0x1f2
		mov al,cl
	out dx,al

	pop eax

	;Step 2: store LBA address to 0x1f3~0x1f6
		
		;0x1f3, low LBA
		mov dx,0x1f3
	out dx,al

		;SHR: shift and rotate
		mov cl,8
	shr eax,cl
		;0x1f4 middle LBA
		mov dx,0x1f4
	out dx,al
		
		;0x1f5 high LBA
		shr eax,cl
		mov dx,0x1f5
	out dx,al

		shr eax,cl
		and al,0000_1111b	
		or  al,1110_0000b		;LBA mode, Master Disk
		;0x1f6 highest four bits
		mov dx,0x1f6
	out dx,al

	;Step 3: send "read"(0x20) command to 0x1f7
		mov al,0x20
		mov dx,0x1f7
	out dx,al

	;Step 4: Polling: detect the status of hard disk
	.ready:
		nop
		in al,dx
		and al,1000_1000b
		cmp al,0000_1000b
	jnz .ready

	;Step 5: read data from 0x1f0 and put it to memory{bx}

		pop ax			;previously in cx(sectors to read)
		mov dx,512/2	;read 2 bytes each time
		mul dx
	mov cx,ax
	.read_data:
		mov dx,0x1f0
	in ax,dx
	mov [bx],ax
		add bx,2
	loop .read_data

	ret
;------------------------------------------------------------
;------------------------------------------------------------
