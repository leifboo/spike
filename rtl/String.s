
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
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	String.0.unboxed.code, .-String.0.unboxed.code
