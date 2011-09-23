
	.text
	.align	4
Function.0.__apply__:
	.globl	Function.0.__apply__
	.type	Function.0.__apply__, @object
	.size	Function.0.__apply__, 4
	.long	Method
	.long	0		# minArgumentCount
	.long	0x80000000	# maxArgumentCount
	.long	0		# localCount
Function.0.__apply__.code:
	.globl	Function.0.__apply__.code
	.type	Function.0.__apply__.code, @function
/* jump to the code represented by the receiver */
	leal	4(%esi), %edi
	jmp	SpikeCallNewMethod
	.size	Function.0.__apply__.code, .-Function.0.__apply__.code
