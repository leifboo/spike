
	.text
	.align	4
BlockContext.0.__apply__:
	.globl	BlockContext.0.__apply__
	.type	BlockContext.0.__apply__, @object
	.size	BlockContext.0.__apply__, 4
	.long	Method
	.long	0		# minArgumentCount
	.long	0x80000000	# maxArgumentCount
	.long	0		# localCount
BlockContext.0.__apply__.code:
	.globl	BlockContext.0.__apply__.code
	.type	BlockContext.0.__apply__.code, @function

/* check argument count */
	cmpl	8(%esi), %edx	# nargs
	je	.L1
	pushl	$__sym_wrongNumberOfArguments
	call	SpikeError

.L1:
/* save BlockContext pointer */
	pushl	%esi

/* get block entry point */
	movl	12(%esi), %eax	# pc

/* restore home context pointer %ebp */
	movl	4(%esi), %ebp

/* restore registers from home context */
	movl	8(%ebp), %ebx
	movl	12(%ebp), %esi
	movl	16(%ebp), %edi

/* jump to the code block */
	jmp	*%eax

	.size	BlockContext.0.__apply__.code, .-BlockContext.0.__apply__.code
