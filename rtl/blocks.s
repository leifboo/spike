
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


SpikeResumeHome:
	.globl	SpikeResumeHome
	.type	SpikeResumeHome, @function

/* save stuff from old stack before we mess with %esp */
	popl	%eax		# return address
	popl	%edx		# result
	popl	%ecx		# BlockContext

/* sabotage BlockContext.pc */
	movl	$SpikeCannotReenterBlock, 12(%ecx)

/* longjmp home */
	movl	4(%ecx), %ebp	# get homeContext
	movl	8(%ebp), %ebx	# restore methodClass
	movl	12(%ebp), %esi	# restore receiver
	movl	16(%ebp), %edi	# restore instVarPointer
	movl	20(%ebp), %esp	# restore stackp
	pushl	%edx		# push result onto home stack
	jmp	*%eax		# return

	.size	SpikeResumeHome, .-SpikeResumeHome


SpikeCannotReenterBlock:
	.globl	SpikeCannotReenterBlock
	.type	SpikeCannotReenterBlock, @function

	pushl	$__sym_cannotReenterBlock
	call	SpikeError

	.size	SpikeCannotReenterBlock, .-SpikeCannotReenterBlock
