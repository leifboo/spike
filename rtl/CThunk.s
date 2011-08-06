
	.text
	.align	4
CThunk.0.__apply__:
	.globl	CThunk.0.__apply__
	.type	CThunk.0.__apply__, @object
	.size	CThunk.0.__apply__, 4
	.long	Method
	.long	0		# minArgumentCount
	.long	~0		# maxArgumentCount
	.long	0		# localCount
CThunk.0.__apply__.code:
	.globl	CThunk.0.__apply__.code
	.type	CThunk.0.__apply__.code, @function

	movl	%ecx, %ebx	# remember arg count

/* copy, reverse, and unbox args */
	movl	$0, %esi	# start at last arg
	jmp	.L2
.L1:
	pushl	8(%ebp,%esi,4)	# push arg
	movl	$__sym_unboxed, %edx
	call	SpikeGetAttr	# unbox it
	movl	%eax, (%esp)	# replace fake obj result with real one
	addl	$1, %esi	# walk back to previous arg
.L2:
	cmpl	%esi, %ebx
	ja	.L1

/* call C function */
	sall	$2, %ebx	# remember size of args
	movl	4(%edi), %eax	# get function pointer
	call	*%eax		# call it
	addl	%ebx, %esp	# clean up C args

/* box result */
	sall	$2, %eax	# assume int result for now
	orl	$2, %eax
	movl	%eax, 8(%ebp,%ebx) # store result

/* clean up */
	movl	%ebx, %ecx	# stash size of args
	popl	%edi		# tear down stack frame
	popl	%esi
	popl	%ebx
	leave
	popl	%eax		# pop return address
	addl	%ecx, %esp	# clean up Spike args
	jmp	*%eax		# return

	.size	CThunk.0.__apply__.code, .-CThunk.0.__apply__.code
