
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

/* copy, reverse, and unbox args */
	movl	$0, %esi	# start at last arg
	jmp	.L2
.L1:
	pushl	64(%ebp,%esi,4)	# push arg
	movl	$__sym_unboxed, %edx
	call	SpikeGetAttr	# unbox arg
	cmpl	$4, %ecx	# replace fake obj result with real one
	je	.L3
	movl	%edx, (%esp)
	pushl	$0
.L3:
	movl	%eax, (%esp)
	addl	$1, %esi	# walk back to previous arg
.L2:
	cmpl	12(%ebp), %esi	# compare with argumentCount
	jb	.L1

/* call C function */
	movl	4(%edi), %eax	# get function pointer
	call	*%eax		# call it
	movl	56(%ebp), %esp	# clean up C args

/* box result */
	pushl	%eax		# arg is Spike pointer (CObject)
	movl	%esp, %eax
	orl	$3, %eax
	pushl	(%edi)		# receiver = signature
	pushl	%eax		# arg
	movl	$__sym_box$, %edx
	movl	$1, %ecx	# argument count
	call	SpikeSendMessage
	movl	12(%ebp), %edx	# get argumentCount
	popl	64(%ebp,%edx,4)	# store result
	popl	%eax		# pop arg storage

/* return */
	ret

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
	movl	%esi, 64(%ebp)	# fake, safe result for Spike code
	movl	4(%edi), %eax	# real result for C/asm code
	movl	$4, %ecx	# result size
	ret
	.size	CFunction.0.unboxed.code, .-CFunction.0.unboxed.code
