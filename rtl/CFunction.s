
	.text
	.align	4
CFunction.0.__apply__:
	.globl	CFunction.0.__apply__
	.type	CFunction.0.__apply__, @object
	.size	CFunction.0.__apply__, 4
	.long	Method
	.long	0		# minArgumentCount
	.long	0x80000000	# maxArgumentCount
	.long	0		# localCount
CFunction.0.__apply__.code:
	.globl	CFunction.0.__apply__.code
	.type	CFunction.0.__apply__.code, @function

	movl	%edx, %ebx	# remember arg count

/* copy, reverse, and unbox args */
	movl	$0, %esi	# start at last arg
	jmp	.L2
.L1:
	pushl	8(%ebp,%esi,4)	# push arg
	movl	$__sym_unboxed, %edx
	call	SpikeGetAttr	# unbox it
	cmpl	$4, %ecx	# replace fake obj result with real one
	je	.L3
	movl	%edx, (%esp)
	pushl	$0
.L3:
	movl	%eax, (%esp)
	addl	$1, %esi	# walk back to previous arg
.L2:
	cmpl	%esi, %ebx
	ja	.L1

/* call C function */
	sall	$2, %ebx	# remember size of args
	movl	4(%edi), %eax	# get function pointer
	call	*%eax		# call it
	leal	-12(%ebp), %esp	# clean up C args

/* box result */
	pushl	%eax		# arg is Spike pointer (CObject)
	movl	%esp, %eax
	orl	$3, %eax
	pushl	(%edi)		# receiver = signature
	pushl	%eax		# arg
	movl	$__sym_box$, %edx
	movl	$1, %ecx	# argument count
	call	SpikeSendMessage
	popl	8(%ebp,%ebx)	# store result
	popl	%eax		# pop arg storage

/* clean up */
	movl	%ebx, %ecx	# stash size of args
	popl	%edi		# tear down stack frame
	popl	%esi
	popl	%ebx
	leave
	popl	%eax		# pop return address
	addl	%ecx, %esp	# clean up Spike args
	jmp	*%eax		# return

	.size	CFunction.0.__apply__.code, .-CFunction.0.__apply__.code


	.text
	.align	4
CFunction.0.unboxed:
	.globl	CFunction.0.unboxed
	.type	CFunction.0.unboxed, @object
	.size	CFunction.0.unboxed, 16
	.long	Method
	.long	0
	.long	0
	.long	0
CFunction.0.unboxed.code:
	.globl	CFunction.0.unboxed.code
	.type	CFunction.0.unboxed.code, @function
	movl	%esi, 8(%ebp)	# fake, safe result for Spike code
	movl	4(%edi), %eax	# real result for C/asm code
	movl	$4, %ecx	# result size
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	CFunction.0.unboxed.code, .-CFunction.0.unboxed.code
