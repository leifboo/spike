
	.text
	.align	8
String.0.unboxed:
	.globl	String.0.unboxed
	.type	String.0.unboxed, @object
	.size	String.0.unboxed, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
String.0.unboxed.code:
	.globl	String.0.unboxed.code
	.type	String.0.unboxed.code, @function
	mov	%rsi, 128(%rbp)	# fake, safe result for Spike code
	lea	8(%rdi), %rax	# real result for C/asm code
	mov	$8, %rcx	# result size
	ret
	.size	String.0.unboxed.code, .-String.0.unboxed.code

