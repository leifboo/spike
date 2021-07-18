
	.text

SpikeRotate:
	.globl	SpikeRotate
	.type	SpikeRotate, @function

	push	%rbp
	mov	%rsp, %rbp

	sub	$1, %rcx	# can't rotate less than 2 args
	jbe	.L2

	add	$16, %rbp	# point to 1st arg
	mov	0(%rbp), %rax	# save it on the stack
	push	%rax

.L1:
	mov	8(%rbp), %rax	# move next N-1 args
	mov	%rax, 0(%rbp)	# towards the top of the stack
	add	$8, %rbp
	loop	.L1

	pop	%rax
	mov	%rax, 0(%rbp)	# place saved arg

.L2:
	pop	%rbp
	ret

	.size	SpikeRotate, .-SpikeRotate
