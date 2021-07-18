
	.text
	.align	8
CObject.0.unboxed:
	.globl	CObject.0.unboxed
	.type	CObject.0.unboxed, @object
	.size	CObject.0.unboxed, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
CObject.0.unboxed.code:
	.globl	CObject.0.unboxed.code
	.type	CObject.0.unboxed.code, @function
	mov	%rsi, 128(%rbp)	# fake, safe result for Spike code is 'self'
	mov	%rsi, %rax	# real result for C/asm code...
	and	$~3, %rax	# ...is also 'self', but unboxed
	mov	$8, %rcx	# result size
	ret
	.size	CObject.0.unboxed.code, .-CObject.0.unboxed.code

