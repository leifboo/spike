
	.text
	.align	4
CObject.0.unboxed:
	.globl	CObject.0.unboxed
	.type	CObject.0.unboxed, @object
	.size	CObject.0.unboxed, 16
	.long	Method
	.long	0
	.long	0
	.long	0
CObject.0.unboxed.code:
	.globl	CObject.0.unboxed.code
	.type	CObject.0.unboxed.code, @function
	movl	%esi, 8(%ebp)	# fake, safe result for Spike code is 'self'
	movl	%esi, %eax	# real result for C/asm code...
	andl	$~3, %eax	# ...is also 'self', but unboxed
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	CObject.0.unboxed.code, .-CObject.0.unboxed.code

