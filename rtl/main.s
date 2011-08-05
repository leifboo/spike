
	.text

main:
	.globl	main
	.type	main, @function

	pushl	$spike.main	# receiver / space for result
	pushl	$0		# XXX: args
	movl	$1, %ecx
	call	SpikeCall	# call 'main' object
	popl	%eax		# get result
	cmpl	$void, %eax	# check for void
	je	.L2
	movl	%eax, %edx
	andl	$3, %edx	# test for SmallInteger
	cmpl	$2, %edx
	je	.L1
	pushl	$__sym_typeError
	call	SpikeError
.L1:
	sarl	$2, %eax	# unbox result
	ret
.L2:
	movl	$0, %eax
	ret

	.size	main, .-main
