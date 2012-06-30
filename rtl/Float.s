
	.text
	.align	4
Float.0.unboxed:
	.globl	Float.0.unboxed
	.type	Float.0.unboxed, @object
	.size	Float.0.unboxed, 16
	.long	Method
	.long	0
	.long	0
	.long	0
Float.0.unboxed.code:
	.globl	Float.0.unboxed.code
	.type	Float.0.unboxed.code, @function
	movl	%esi, 8(%ebp)	# fake, safe result for Spike code
	movl	0(%edi), %eax	# real result for C/asm code (low)
	movl	4(%edi), %edx	#   (high)
	movl	$8, %ecx	#   (size)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	Float.0.unboxed.code, .-Float.0.unboxed.code


	.text
	.align	4
Float.0.asCObject:
	.globl	Float.0.asCObject
	.type	Float.0.asCObject, @object
	.size	Float.0.asCObject, 16
	.long	Method
	.long	0
	.long	0
	.long	0
Float.0.asCObject.code:
	.globl	Float.0.asCObject.code
	.type	Float.0.asCObject.code, @function
	orl	$3, %esi	# map to CObject
	movl	%esi, 8(%ebp)	# store result
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	Float.0.asCObject.code, .-Float.0.asCObject.code
