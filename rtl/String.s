
	.text
	.align	4
String.0.unboxed:
	.globl	String.0.unboxed
	.type	String.0.unboxed, @object
	.size	String.0.unboxed, 16
	.long	Method
	.long	0
	.long	0
	.long	0
String.0.unboxed.code:
	.globl	String.0.unboxed.code
	.type	String.0.unboxed.code, @function
	movl	%esi, 8(%ebp)	# fake, safe result for Spike code
	leal	4(%edi), %eax	# real result for C/asm code
	movl	$4, %ecx	# result size
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	String.0.unboxed.code, .-String.0.unboxed.code


	.text
	.align	4
String.0.asCObject:
	.globl	String.0.asCObject
	.type	String.0.asCObject, @object
	.size	String.0.asCObject, 16
	.long	Method
	.long	0
	.long	0
	.long	0
String.0.asCObject.code:
	.globl	String.0.asCObject.code
	.type	String.0.asCObject.code, @function
	orl	$3, %esi	# map to CObject
	movl	%esi, 8(%ebp)	# store result
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	String.0.asCObject.code, .-String.0.asCObject.code
