
	.text

main:
	.globl	main
	.type	main, @function

	call	SpikeInstallTrapHandler

/* allocate and initialize the first Context */
	leal	-64(%esp), %esp
	movl	$0, %eax
	movl	$MethodContext, (%esp)	# -> klass is-a pointer
	movl	%eax, 4(%esp)	# 0 -> caller
	movl	%esp, 8(%esp)	# -> homeContext
	movl	%eax, 12(%esp)	# 0 -> argumentCount
	movl	%eax, 16(%esp)	# 0 -> pc
	movl	%eax, 20(%esp)	# 0 -> sp
	movl	%eax, 24(%esp)	# 0 -> regSaveArea[0]
	movl	%eax, 28(%esp)	# 0 -> regSaveArea[1]
	movl	%eax, 32(%esp)	# 0 -> regSaveArea[2]
	movl	%eax, 36(%esp)	# 0 -> regSaveArea[3]
	movl	%eax, 40(%esp)	# 0 -> method
	movl	%eax, 44(%esp)	# 0 -> methodClass
	movl	%eax, 48(%esp)	# 0 -> receiver
	movl	%eax, 52(%esp)	# 0 -> instVarPointer
	movl	%esp, 56(%esp)	# -> stackBase
	movl	%eax, 60(%esp)	# 0 -> size
	movl	%esp, %ebp
	movl	%ebp, %ebx

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
	leal	64(%esp), %esp
	ret
.L2:
	movl	$0, %eax
	leal	64(%esp), %esp
	ret

	.size	main, .-main
