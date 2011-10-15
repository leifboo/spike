
	.text

SpikeBlockCopy:
	.globl	SpikeBlockCopy
	.type	SpikeBlockCopy, @function

	movl	(%esp), %eax	# get return address
	addl	$2, %eax	# account for branch
	movl	%eax, (%esp)	# set startpc

	call	SpikeCreateBlockContext

	movl	%eax, 8(%esp)	# save new BlockContext
	popl	%eax		# pop startpc
	subl	$2, %eax	# account for branch
	addl	$4, %esp	# pop nargs
	jmp	*%eax		# return

	.size	SpikeBlockCopy, .-SpikeBlockCopy


SpikeResumeCaller:
	.globl	SpikeResumeCaller
	.type	SpikeResumeCaller, @function

	popl	12(%ecx)	# pop return address into BlockContext.pc
	jmp	*%eax		# return to block's caller

	.size	SpikeResumeCaller, .-SpikeResumeCaller
