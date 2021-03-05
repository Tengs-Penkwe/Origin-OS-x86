	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 11, 0	sdk_version 11, 1
	.globl	_main                   ## -- Begin function main
	.p2align	4, 0x90
_main:                                  ## @main
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	$0, -4(%rbp)
	movl	$123, -8(%rbp)
	movl	$1312, -12(%rbp)        ## imm = 0x520
LBB0_1:                                 ## =>This Inner Loop Header: Depth=1
	movl	-8(%rbp), %eax
	addl	-12(%rbp), %eax
	movl	%eax, -16(%rbp)
	jmp	LBB0_1
	.cfi_endproc
                                        ## -- End function
.subsections_via_symbols

	.section        .note.GNU-stack,"",@progbits
