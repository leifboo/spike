
	.text
	.align	4
Float.0.unboxed:
	.globl	Float.0.unboxed
	.type	Float.0.unboxed, @object
	.size	Float.0.unboxed, 16
	.long	__spk_x_Method
	.long	0
	.long	0
	.long	0
Float.0.unboxed.code:
	.globl	Float.0.unboxed.code
	.type	Float.0.unboxed.code, @function
	movl	%esi, 64(%ebp)	# fake, safe result for Spike code
	movl	0(%edi), %eax	# real result for C/asm code (low)
	movl	4(%edi), %edx	#   (high)
	movl	$8, %ecx	#   (size)
	ret
	.size	Float.0.unboxed.code, .-Float.0.unboxed.code

