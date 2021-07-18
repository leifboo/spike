
	.text

SpikeBlockCopy:
	.globl	SpikeBlockCopy
	.type	SpikeBlockCopy, @function

	mov	(%rsp), %rax	# get return address
	add	$2, %rax	# account for branch
	mov	%rax, (%rsp)	# set pc

	mov	0(%rsp), %rdi
	mov	8(%rsp), %rsi
	mov	16(%rsp), %rdx
	call	SpikeCreateBlockContext
	mov	104(%rbp), %rdi	# restore regs
	mov	96(%rbp), %rsi

	mov	%rax, 16(%rsp)	# save new BlockContext
	pop	%rax		# pop pc
	sub	$2, %rax	# account for branch
	add	$8, %rsp	# pop argumentCount
	jmp	*%rax		# return

	.size	SpikeBlockCopy, .-SpikeBlockCopy


SpikeYield:
	.globl	SpikeYield
	.type	SpikeYield, @function

/* pop return address into BlockContext.pc */
	pop	32(%rbp)

/* set activeContext to the caller */
	mov	8(%rbp), %rbp

/* restore registers */
	mov	48(%rbp), %rbx
	mov	56(%rbp), %rsi
	mov	64(%rbp), %rdi

/* return */
	mov	32(%rbp), %rax	# pc
	jmp	*%rax

	.size	SpikeYield, .-SpikeYield
