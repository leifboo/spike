
	.text
	.align	4
Function.0.__apply__:
	.globl	Function.0.__apply__
	.type	Function.0.__apply__, @object
	.size	Function.0.__apply__, 4
	.long	0		# minArgumentCount
	.long	~0		# maxArgumentCount
	.long	0		# localCount
	.long	Method
Function.0.__apply__.code:
	.globl	Function.0.__apply__.code
	.type	Function.0.__apply__.code, @function
/* jump to the code represented by the receiver */
	movl	%esi, %eax
	jmp	SpikeCallNewMethod
	.size	Function.0.__apply__.code, .-Function.0.__apply__.code
