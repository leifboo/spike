
	.text
	.align	4
Char.0.unboxed:
	.globl	Char.0.unboxed
	.type	Char.0.unboxed, @object
	.size	Char.0.unboxed, 16
	.long	__spk_x_Method
	.long	0
	.long	0
	.long	0
Char.0.unboxed.code:
	.globl	Char.0.unboxed.code
	.type	Char.0.unboxed.code, @function
	movl	%esi, 64(%ebp)	# fake, safe result for Spike code
	movl	(%edi), %eax	# real result for C/asm code
	movl	$4, %ecx	# result size
	ret
	.size	Char.0.unboxed.code, .-Char.0.unboxed.code
