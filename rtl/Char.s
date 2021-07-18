
	.text
	.align	8
Char.0.unboxed:
	.globl	Char.0.unboxed
	.type	Char.0.unboxed, @object
	.size	Char.0.unboxed, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
Char.0.unboxed.code:
	.globl	Char.0.unboxed.code
	.type	Char.0.unboxed.code, @function
	mov	%rsi, 128(%rbp)	# fake, safe result for Spike code
	mov	(%rdi), %rax	# real result for C/asm code
	mov	$8, %rcx	# result size
	ret
	.size	Char.0.unboxed.code, .-Char.0.unboxed.code
