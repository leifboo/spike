
/* Pop a single object off the stack.
   Set Z = 0 if arg is false, Z = 1 if arg is true, error otherwise */
	
	.text

SpikeTest:
	.globl	SpikeTest
	.type	SpikeTest, @function

	movl	4(%esp), %eax	# get arg

	cmpl	$true, %eax
	jne	.L2
	cmpl	$false, %eax	# Z = 1
.L1:
	popl	%eax		# pop return address
	movl	%eax, 0(%esp)	# replace arg with RA
	ret
.L2:
	cmpl	$false, %eax
	je	.L1
	cmpl	$null, %eax
	je	.L1

/* XXX: accept 0, 0.0, (hook?) */

	pushl	$__sym_mustBeBoolean
	call	SpikeError

	.size	SpikeTest, .-SpikeTest
