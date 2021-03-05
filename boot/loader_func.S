;------------------------------------------------------------
;INT 0x15    功能号:EAX->0xE820   EDX->0x554D4150('SMAP')
;功能描述:Detect Memory Size
;------------------------------------------------------------
;EAX = 0xE820
;EDX = ASCII	:0x554D4150('SMAP')
;EBX = The next place of ARDS
	;set as 0 in the first, then updates automatically
;ECX = the size of struct ARDS(20 as default)
;ES:DI	= ARDS buffer

;CF	 = 0 indicates succeed, 1 failed
;EAX = ASCII	:0x554D4150('SMAP')
;EBX = The next place of ARDS
	;return 0 indicates this is the last ARDS sruct
;ECX = the size of struct ARDS(20 as default)
;ES:DI	= ARDS written here

	;-------------------------------------------
	;enter the syscall
	;-------------------------------------------
		;the base address of ARDS is 0
		xor ebx,ebx
		;
		mov edx,0x554D4150		;SMAP
		;
		mov di,ards_buf

	.e820_mem_get_loop:
		mov eax,0xe820
		mov ecx,20
	int 0x15

	jc .e820_failed_try_e801

		add di,cx				;di points to the new place of ARDS
		inc word [ards_nr]		;record how many ARDS are returned

		cmp ebx,0				;=0 equals end
	jnz .e820_mem_get_loop

	;-------------------------------------------
	;find the biggest value in ARDS
	;bubble sort
	;-------------------------------------------
	;edx : store the biggest value
		xor edx,edx
		mov ebx,ards_buf

	mov cx,[ards_nr]
	.find_max_mem_area:
			mov eax,[ebx]
			add eax,[ebx+8]
		add ebx,20
			cmp edx,eax
		jge .next_ards
			mov edx,eax
		.next_ards:	
	loop .find_max_mem_area

	jmp .mem_get_ok

;------------------------------------------------------------
;INT 0x15    功能号:EAX->0xE801 
;功能描述:Detect Memory Size
;------------------------------------------------------------
;EAX = 0xE801

;CF	 = 0 indicates succeed, 1 failed
;AX  = Unit: 1KiB, only return memory under 15MiB, 0x3c00 is maxium
;CX  = AX 
;BX  = Unit: 64KiB, inside 16Mib~4GiB
;DX  = BX

	.e820_failed_try_e801:
		mov ax,0xe801
		int 0x15
	jc .e801_failed_try_88

	;calculate the memory under 15MiB
	;cx as multipler
	;esi as temporary store

			mov cx,0x400
		mul cx
			shl edx,16
			and eax,0x0000_FFFF
			or  edx,eax
		add edx,0x10_0000

		mov esi, edx
	;calculate the memory bewtween 16MiB~4GiB
	;ecx as multipler
	;esi as temporary store
			xor eax,eax
		mov ax,bx

			mov ecx,0x10_0000
		mul ecx
		add esi,eax
	
	mov edx,esi
	jmp .mem_get_ok

;------------------------------------------------------------
;INT 0x15    功能号:AH->0x88
;功能描述:Detect Memory Size
;------------------------------------------------------------
;EAX = 0x88

;CF	 = 0 indicates succeed, 1 failed
;AX  = Unit: 1KiB, Above 1MiB

		mov ah,0x88
		int 0x15

	.e801_failed_try_88:

			mov cx,0x400
		mul cx
			shl edx,16
			or  edx,eax
		add edx,0x10_0000
	
;------------------------------------------------------------
	.mem_get_ok:
		mov [total_mem_bytes], edx
	
;------------------------------------------------------------
setup_page:
;Create Page Directory and Page Tables
;PD 1M ~ 1M+4K, PTs follow it
;------------------------------------------------------------
;no paramaters

;no return
	
	;clean up memory
	;------------------
	mov ecx,4096
		mov esi,0
	.clear_page_dir:
		mov byte [PAGE_DIR_TABLE_POS + esi],0
		inc esi
	loop .clear_page_dir

	;Page Directory Entry
	;------------------
		;location of a page table (no order)
		mov eax,PAGE_DIR_TABLE_POS + 0x1000
		or  eax,PG_US_U | PG_RW_W | PG_P	;User, Writable, In-memory
		
		;let 0x0 ~ 0x40_000(4M)-1 points to the PT above
		mov [PAGE_DIR_TABLE_POS + 0x0],eax
		;let 0xc000_0000(3G) ~ 0xc040_000(3G + 4M)-1 points to the PT above
		mov [PAGE_DIR_TABLE_POS + 0xc00],eax

		;Page Directory is also a Page Table
		sub eax,0x1000
		;let 4G-4M ~ 4G points to this PT
		mov [PAGE_DIR_TABLE_POS + 4092],eax

	;Page Table Entry
	;------------------
		;a normal PT, no order 
		mov ebx,PAGE_DIR_TABLE_POS + 0x1000
		;Physical Address
		;points to 0~4K
		mov edx,PG_US_U | PG_RW_W | PG_P
			mov esi,0
		;only map 0~1M to the PT(4M) above 
		mov ecx,256								;1M/4K = 256 
		.create_pte:
			mov [ebx + esi*4],edx
			add edx,4096
			inc esi	
		loop .create_pte

	;PDE of 3G+4M ~ 4G-4M
	;kernel space, shared between users
	;------------------
		;a normal PT
		mov eax,PAGE_DIR_TABLE_POS + 0x2000
		or  eax,PG_US_U | PG_RW_W | PG_P
		
		;let PTs take their place
		;here we choose to let PT follow closely after PD in memory
		;make PT address =  PD address + PD-order * 4096 + 4096(PD) + 4096(PT) 
		mov ebx,PAGE_DIR_TABLE_POS
			mov esi,769
		mov ecx,254
		.create_kernel_pde:
			mov [ebx + esi*4],eax
			inc esi
			add eax,0x1000
		loop .create_kernel_pde

ret

;---------------- Copy Memory by byte ---------------------
;---------------- mem_cpy (dst, src, size)---------------------
;Input: dstination, source, size by stack
;Output: no
;------------------------------------------------------------
mem_cpy:
		
			push ebp
		mov ebp, esp
		push ecx

		cld
		mov edi, [ebp + 8]		;dest
		mov esi, [ebp + 12]		;src
		mov ecx, [ebp + 16] 	;size
mov byte [gs:160],'9'
		rep movsb
mov byte [gs:160],'0'

		pop ecx
		pop ebx
ret
;------------------------------------------------------------


;------------------------------------------------------------
;copy kernel to the right place
;kernel is load in KERNEL_BIN_BASE_ADDR as ELF file
;load the segment to right place
;------------------------------------------------------------
;ebx = Address of Program Header
;ecx = Number of Program Header		;
;edx = Size of Program Header		;e_phentsize



kernel_init:
		
			xor edx,edx
		mov dx, [KERNEL_BIN_BASE_ADDR + 42]		;e_phentsize, the size of program header

			mov ebx, [KERNEL_BIN_BASE_ADDR + 28];e_phoff, the offset of first PH
		add ebx, KERNEL_BIN_BASE_ADDR		

	mov cx, [KERNEL_BIN_BASE_ADDR + 44]			;e_phnum, the number of PH
	
	.each_segment:
		; =0 indicates the PH is not used
		cmp byte [ebx + 0],PT_NULL		
mov byte [gs:160],'1'
	je .PTNULL

mov byte [gs:160],'2'
		;size 
		push dword [ebx + 16]		;p_filesz ; size of this PH				
		;src
			mov eax, [ebx + 4]
			add eax, KERNEL_BIN_BASE_ADDR
		push eax
		;dest
		push dword [ebx + 8]
mov byte [gs:160],'3'
	
	call mem_cpy
			add esp,12

mov byte [gs:160],'4'
	.PTNULL:
		;point to next PH
		add	ebx, edx						;size of PH
	loop .each_segment

ret 
;------------------------------------------------------------

;------------------------------------------------------------
;Read n Sectors in Hard Disk
_read_n_disk_32:
;------------------------------------------------------------
;EAX	= LBA sector
;EBX	= Memory Address to Write
;ECX	= Sectors to Read
;No Return Value

	push ecx

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

		pop eax			;previously in ecx(sectors to read)
		mov dx,512/2	;read 2 bytes each time
		mul dx
	mov cx,ax
	.read_data:
		mov dx,0x1f0
	in ax,dx
	mov [ebx],ax
		add ebx,2
	loop .read_data

	ret
;------------------------------------------------------------

