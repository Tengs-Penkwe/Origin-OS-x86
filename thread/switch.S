[bits 32]
SECTION .text

global switch_to
switch_to:
	;----------------------------------------
	;Save the Environment of Current Thread
	;----------------------------------------
		;Next Task PCB Address -> self_kstack
		;Current Task 
		;Return Address in Stack
		push esi
		push edi
		push ebx
		push ebp

		mov eax,[esp+20]
	mov [eax], esp		;cur->self_kstack

	;----------------------------------------
	;Recover the Environment of Next Thread
	;----------------------------------------
		mov eax,[esp+24]
	mov esp, [eax]		;0 stack pointer

		pop ebp
		pop ebx
		pop edi
		pop esi
ret
