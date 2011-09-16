
	.text

SpikeRotate:
	.globl	SpikeRotate
	.type	SpikeRotate, @function

	pushl	%ebp
	movl	%esp, %ebp

	subl	$1, %ecx	# can't rotate less than 2 args
	jbe	.L2

	addl	$8, %ebp	# point to 1st arg
	movl	0(%ebp), %eax	# save it on the stack
	pushl	%eax

.L1:
	movl	4(%ebp), %eax	# move next N-1 args
	movl	%eax, 0(%ebp)	# towards the top of the stack
	addl	$4, %ebp
	loop	.L1

	popl	%eax
	movl	%eax, 0(%ebp)	# place saved arg

.L2:
	popl	%ebp
	ret

	.size	SpikeRotate, .-SpikeRotate
