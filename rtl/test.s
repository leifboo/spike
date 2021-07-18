
/* Pop a single object off the stack.
   Set Z = 0 if arg is false, Z = 1 if arg is true, error otherwise */
	
	.text

SpikeTest:
	.globl	SpikeTest
	.type	SpikeTest, @function

	mov	8(%rsp), %rax	# get arg

	cmp	$__spk_x_true, %rax
	jne	.L2
	cmp	$__spk_x_false, %rax	# Z = 1
.L1:
	pop	%rax		# pop return address
	mov	%rax, 0(%rsp)	# replace arg with RA
	ret
.L2:
	cmp	$__spk_x_false, %rax
	je	.L1
	cmp	$0, %rax	# null
	je	.L1

/* XXX: accept 0, 0.0, (hook?) */

	push	$__spk_sym_mustBeBoolean
	call	SpikeError

	.size	SpikeTest, .-SpikeTest
