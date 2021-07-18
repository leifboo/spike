
	.text
	.align	8
BlockContext.0.__apply__:
	.globl	BlockContext.0.__apply__
	.type	BlockContext.0.__apply__, @object
	.size	BlockContext.0.__apply__, 32
	.quad	__spk_x_Method
	.quad	0		# minArgumentCount
	.quad	0x80000000	# maxArgumentCount
	.quad	0		# localCount
BlockContext.0.__apply__.code:
	.globl	BlockContext.0.__apply__.code
	.type	BlockContext.0.__apply__.code, @function

/* check argument count */
	mov	24(%rbp), %rdx	# thisContext.argumentCount
	cmp	24(%rsi), %rdx	#   == self.argumentCount ?
	je	.L1
	push	$__spk_sym_wrongNumberOfArguments
	call	SpikeError

.L1:
/* move 'caller' from our MethodContext to the receiver */
	mov	8(%rbp), %rax
	mov	%rax, 8(%rsi)

/* discard our own MethodContext */
	lea	128(%rbp), %rsp

/* set activeContext to the receiver */
	mov	%rsi, %rbp

/* restore registers */
	mov	48(%rbp), %rbx
	mov	56(%rbp), %rsi
	mov	64(%rbp), %rdi

/* jump to the code block */
	mov	32(%rbp), %rax	# pc
	jmp	*%rax

	.size	BlockContext.0.__apply__.code, .-BlockContext.0.__apply__.code


	.text
	.align	8
BlockContext.0.closure:
	.globl	BlockContext.0.closure
	.type	BlockContext.0.closure, @object
	.size	BlockContext.0.closure, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
BlockContext.0.closure.code:
	.globl	BlockContext.0.closure.code
	.type	BlockContext.0.closure.code, @function
	mov	%rsi, %rdi
	call	SpikeCreateClosure
	mov	%rax, 128(%rbx)
	ret
	.size	BlockContext.0.closure.code, .-BlockContext.0.closure.code

