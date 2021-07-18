
	.text
	.align	8
Function.0.__apply__:
	.globl	Function.0.__apply__
	.type	Function.0.__apply__, @object
	.size	Function.0.__apply__, 32
	.quad	__spk_x_Method
	.quad	0		# minArgumentCount
	.quad	0x80000000	# maxArgumentCount
	.quad	0		# localCount
Function.0.__apply__.code:
	.globl	Function.0.__apply__.code
	.type	Function.0.__apply__.code, @function

/* discard our own context */
	mov	24(%rbp), %rdx	# get argumentCount
	mov	8(%rbp), %rbp	# activeContext = caller
	lea	128(%rbx), %rsp	# pop context

/* jump to the code represented by the receiver */
	mov	(%rsi), %rbx	# methodClass = my class
	lea	8(%rsi), %rdi	# method = self
	jmp	SpikePrologue

	.size	Function.0.__apply__.code, .-Function.0.__apply__.code
