	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 13
	.section	__TEXT,__literal8,8byte_literals
	.p2align	3               ## -- Begin function main
LCPI0_0:
	.quad	-4611686018427387904    ## double -2
LCPI0_1:
	.quad	4607182418800017408     ## double 1
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_main
	.p2align	4, 0x90
_main:                                  ## @main
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$80, %rsp
	leaq	L_.str(%rip), %rax
	movsd	LCPI0_1(%rip), %xmm0    ## xmm0 = mem[0],zero
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	movsd	%xmm0, -24(%rbp)
	movq	%rax, %rdi
	movb	$0, %al
	callq	_printf
	leaq	L_.str.1(%rip), %rdi
	movsd	LCPI0_1(%rip), %xmm0    ## xmm0 = mem[0],zero
	movq	$1, -32(%rbp)
	movq	-32(%rbp), %rsi
	movq	-32(%rbp), %rdx
	movl	%eax, -60(%rbp)         ## 4-byte Spill
	movb	$1, %al
	callq	_printf
	leaq	L_.str.2(%rip), %rdi
	movsd	LCPI0_0(%rip), %xmm0    ## xmm0 = mem[0],zero
	movl	$4294967295, %ecx       ## imm = 0xFFFFFFFF
	movl	%ecx, %edx
	movsd	%xmm0, -40(%rbp)
	movq	$-2147483648, -48(%rbp) ## imm = 0x80000000
	movq	-48(%rbp), %rsi
	sarq	$32, %rsi
	movl	%esi, %ecx
	movl	%ecx, -52(%rbp)
	andq	-48(%rbp), %rdx
	movl	%edx, %ecx
	movl	%ecx, -56(%rbp)
	movq	-48(%rbp), %rsi
	movl	-52(%rbp), %edx
	movl	-56(%rbp), %ecx
	movl	%eax, -64(%rbp)         ## 4-byte Spill
	movb	$1, %al
	callq	_printf
	xorl	%ecx, %ecx
	movl	%eax, -68(%rbp)         ## 4-byte Spill
	movl	%ecx, %eax
	addq	$80, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"good\n"

L_.str.1:                               ## @.str.1
	.asciz	"z = %f, y = %lld = %llx\n"

L_.str.2:                               ## @.str.2
	.asciz	"val = %f, conv = 0x%llx = %x-%x\n"


.subsections_via_symbols
