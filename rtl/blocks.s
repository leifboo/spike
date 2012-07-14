
	.text

SpikeBlockCopy:
	.globl	SpikeBlockCopy
	.type	SpikeBlockCopy, @function

	movl	(%esp), %eax	# get return address
	addl	$2, %eax	# account for branch
	movl	%eax, (%esp)	# set pc

	call	SpikeCreateBlockContext

	movl	%eax, 8(%esp)	# save new BlockContext
	popl	%eax		# pop pc
	subl	$2, %eax	# account for branch
	addl	$4, %esp	# pop argumentCount
	jmp	*%eax		# return

	.size	SpikeBlockCopy, .-SpikeBlockCopy


SpikeYield:
	.globl	SpikeYield
	.type	SpikeYield, @function

/* pop return address into BlockContext.pc */
	popl	16(%ebp)

/* set activeContext to the caller */
	movl	4(%ebp), %ebp

/* restore registers */
	movl	24(%ebp), %ebx
	movl	28(%ebp), %esi
	movl	32(%ebp), %edi

/* return */
	movl	16(%ebp), %eax	# pc
	jmp	*%eax

	.size	SpikeYield, .-SpikeYield
