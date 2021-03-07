%include "kernel.inc"

section .data
put_int_buffer_test   dq    0     ; 定义8字节缓冲区用于数字到字符的转换

[bits 32]
section .text

put_int_buffer    dq    0     ; 定义8字节缓冲区用于数字到字符的转换
;--------------------   将小端字节序的数字变成对应的ascii后，倒置   -----------------------
;输入：栈中参数为待打印的数字
;输出：在屏幕上打印16进制数字,并不会打印前缀0x,如打印10进制15时，只会直接打印f，不会是0xf
;------------------------------------------------------------------------------------------
global put_int
put_int:
   pushad
   mov ebp, esp
   mov eax, [ebp+4*9]		       ; call的返回地址占4字节+pushad的8个4字节
   mov edx, eax
   mov edi, 7                          ; 指定在put_int_buffer中初始的偏移量
   mov ecx, 8			       ; 32位数字中,16进制数字的位数是8个
   mov ebx, put_int_buffer

;将32位数字按照16进制的形式从低位到高位逐个处理,共处理8个16进制数字
.16based_4bits:			       ; 每4位二进制是16进制数字的1位,遍历每一位16进制数字
   and edx, 0x0000000F		       ; 解析16进制数字的每一位。and与操作后,edx只有低4位有效
   cmp edx, 9			       ; 数字0～9和a~f需要分别处理成对应的字符
   jg .is_A2F 
   add edx, '0'			       ; ascii码是8位大小。add求和操作后,edx低8位有效。
   jmp .store
.is_A2F:
   sub edx, 10			       ; A~F 减去10 所得到的差,再加上字符A的ascii码,便是A~F对应的ascii码
   add edx, 'A'

;将每一位数字转换成对应的字符后,按照类似“大端”的顺序存储到缓冲区put_int_buffer
;高位字符放在低地址,低位字符要放在高地址,这样和大端字节序类似,只不过咱们这里是字符序.
.store:
; 此时dl中应该是数字对应的字符的ascii码
   mov [ebx+edi], dl		       
   dec edi
   shr eax, 4
   mov edx, eax 
   loop .16based_4bits

;现在put_int_buffer中已全是字符,打印之前,
;把高位连续的字符去掉,比如把字符000123变成123
.ready_to_print:
   inc edi			       ; 此时edi退减为-1(0xffffffff),加1使其为0
.skip_prefix_0:  
   cmp edi,8			       ; 若已经比较第9个字符了，表示待打印的字符串为全0 
   je .full0 
;找出连续的0字符, edi做为非0的最高位字符的偏移
.go_on_skip:   
   mov cl, [put_int_buffer+edi]
   inc edi
   cmp cl, '0' 
   je .skip_prefix_0		       ; 继续判断下一位字符是否为字符0(不是数字0)
   dec edi			       ;edi在上面的inc操作中指向了下一个字符,若当前字符不为'0',要恢复edi指向当前字符		       
   jmp .put_each_num

.full0:
   mov cl,'0'			       ; 输入的数字为全0时，则只打印0
.put_each_num:
   push ecx			       ; 此时cl中为可打印的字符
   call put_char
   add esp, 4
   inc edi			       ; 使edi指向下一个字符
   mov cl, [put_int_buffer+edi]	       ; 获取下一个字符到cl寄存器
   cmp edi,8
   jl .put_each_num
   popad
   ret



;------------------------------ put_int_2 ------------------------------
global	put_int_2
;transform number inside stack to ASCII(without prefix) and print it to cursor
;the number is in little endian
		;----------------------------------------
		;Buffer to store processed integer
		buf	  	dq 0, 0

		;----------------------------------------
		;lookup table
		lut		db "0","1","2","3","4","5","6","7" 
				db "8","9","a","b","c","d","e","f"
put_int_2:
;Buf = store processed int
;EDX = store int when processing
;EBX = place of cursor, also where to put string
;----------------------------------------------------------------------
;ESP = the first byte in the stack points to the integer 

;#not implement yet
;EAX = -1 indicate failure, 0~128 indicate the char we print

			pushad
			mov ebp, esp

			;mov int to edx
			mov edx,[ebp + 8*4 +4]

			sub ebp,12

			;ebx points to buffer
			mov ebx,lut


		;----------------------------------------
		;get ascii from int
		;----------------------------------------
		mov ecx,8					;32bits -> 8 ASCII
		.lookup:
				xor eax,eax
				mov al,dl
				and al,0000_1111b
			add ebx,eax
		
			;push
			mov esi, eax

				mov al,[ebx]
			mov byte [ebp + ecx ], al
			;pop
			mov eax,esi

				shr edx,4
			sub ebx,eax
		loop .lookup

		;----------------------------------------
		;print it
		;----------------------------------------
				mov byte [ebp + 8],0

				sub esp,12
			call put_str
				add esp,12		;not needed

		popad 
ret
			

;----------------------------------------------------------------------


;------------------------------ put_str ------------------------------
global	put_str
;put a string in stack to cursor
put_str:
;EBX = place of cursor, also where to put string
;----------------------------------------------------------------------
;ESP = the first byte in the stack points to the char; since we pushad, esp+32+4
;	   the string should be end with '\0' 

;#not implemented yet
;EAX = -1 indicate failure, 0~128 indicate the char we print

		push ebx
		push ecx

		xor ecx,ecx
		;pointer to string
		mov ebx, [esp+12]

	.puts:
		mov cl, [ebx]
	cmp cl,0
	jz .str_over
		
			push ecx
		call put_char
			add esp,4
		
		inc ebx
	jmp .puts

	.str_over:
		pop ecx
		pop ebx
ret
;----------------------------------------------------------------------


;------------------------------ put_char ------------------------------
global	put_char
;put a charactor in stack to cursor
put_char:
;EBX = place of cursor, also where to put char
;----------------------------------------------------------------------
;ESP = the first byte in the stack points to the char; since we pushad, esp+32+4

;#not implemented yet
;EAX = -1 indicate failure, 0~128 indicate the char we print

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
		mov byte [gs:bx],0x07
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

		.done_is_carrage_return:
			add bx,80
			cmp bx,2000
		.done_is_line_feed:
			jl .set_cursor

;--------------------------------------------------
.roll_screen:
;--------------------------------------------------

		;------------------------------
		;copy 1~24 to 0~23
		;------------------------------
			
				cld
			mov ecx, 960		;3840/4 = 960
			mov esi,KERNEL_MAP_BASE_ADDR + 0xb800 + 160
			mov edi,KERNEL_MAP_BASE_ADDR + 0xb800 + 0
			rep movsd

		;------------------------------
		;clear line 24
		;------------------------------
			mov ebx,3840
		mov ecx,80
		.cls:
			mov word [gs:ebx], 0x0720		;0x7 black background + white char
			add ebx,2
		loop .cls
			mov ebx,1920

;--------------------------------------------------
.set_cursor:
;--------------------------------------------------
		
		;------------------------------
		;set higher 8 
		;------------------------------
			mov dx, 0x3d4
			mov al, 0x0e
		out dx,al
			mov dx, 0x3d5
			mov al,bh
		out dx,al

		;------------------------------
		;set lower 8 
		;------------------------------
			mov dx, 0x3d4
			mov al, 0x0f
		out dx,al
			mov dx, 0x3d5
			mov al, bl
		out dx,al

		.done_put_char:
popad
ret

;--------------------------------------------------------------------------------




















