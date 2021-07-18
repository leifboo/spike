
	.text
	.align	8
Float.0.unboxed:
	.globl	Float.0.unboxed
	.type	Float.0.unboxed, @object
	.size	Float.0.unboxed, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
Float.0.unboxed.code:
	.globl	Float.0.unboxed.code
	.type	Float.0.unboxed.code, @function
	mov	%rsi, 128(%rbp)	# fake, safe result for Spike code
	mov	(%rdi), %rax	# real result for C/asm code
	mov	$8, %rcx	# result size
	ret
	.size	Float.0.unboxed.code, .-Float.0.unboxed.code

