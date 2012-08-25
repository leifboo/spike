
	.text
	.align	4
BlockContext.0.__apply__:
	.globl	BlockContext.0.__apply__
	.type	BlockContext.0.__apply__, @object
	.size	BlockContext.0.__apply__, 4
	.long	__spk_x_Method
	.long	0		# minArgumentCount
	.long	0x80000000	# maxArgumentCount
	.long	0		# localCount
BlockContext.0.__apply__.code:
	.globl	BlockContext.0.__apply__.code
	.type	BlockContext.0.__apply__.code, @function

/* check argument count */
	movl	12(%ebp), %edx	# thisContext.argumentCount
	cmpl	12(%esi), %edx	#   == self.argumentCount ?
	je	.L1
	pushl	$__spk_sym_wrongNumberOfArguments
	call	SpikeError

.L1:
/* move 'caller' from our MethodContext to the receiver */
	movl	4(%ebp), %eax
	movl	%eax, 4(%esi)

/* discard our own MethodContext */
	leal	64(%ebp), %esp

/* set activeContext to the receiver */
	movl	%esi, %ebp

/* restore registers */
	movl	24(%ebp), %ebx
	movl	28(%ebp), %esi
	movl	32(%ebp), %edi

/* jump to the code block */
	movl	16(%ebp), %eax	# pc
	jmp	*%eax

	.size	BlockContext.0.__apply__.code, .-BlockContext.0.__apply__.code


	.text
	.align	4
BlockContext.0.closure:
	.globl	BlockContext.0.closure
	.type	BlockContext.0.closure, @object
	.size	BlockContext.0.closure, 16
	.long	__spk_x_Method
	.long	0
	.long	0
	.long	0
BlockContext.0.closure.code:
	.globl	BlockContext.0.closure.code
	.type	BlockContext.0.closure.code, @function
	pushl	%esi
	call	SpikeCreateClosure
	addl	$4, %esp
	movl	%eax, 64(%ebx)
	ret
	.size	BlockContext.0.closure.code, .-BlockContext.0.closure.code

