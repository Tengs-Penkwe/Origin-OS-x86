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
