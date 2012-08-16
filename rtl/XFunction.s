
	.text
	.align	4
XFunction.0.__apply__:
	.globl	XFunction.0.__apply__
	.type	XFunction.0.__apply__, @object
	.size	XFunction.0.__apply__, 4
	.long	Method
	.long	0		# minArgumentCount
	.long	0x80000000	# maxArgumentCount
	.long	0		# localCount
XFunction.0.__apply__.code:
	.globl	XFunction.0.__apply__.code
	.type	XFunction.0.__apply__.code, @function

/* copy and reverse args */
	movl	$0, %esi	# start at last arg
	jmp	.L2
.L1:
	pushl	64(%ebp,%esi,4)	# push arg
	addl	$1, %esi	# walk back to previous arg
.L2:
	cmpl	12(%ebp), %esi	# compare with argumentCount
	jb	.L1

/* call C function */
	movl	4(%edi), %eax	# get function pointer
	call	*%eax		# call it
	movl	56(%ebp), %esp	# clean up C args

/* store result */
	movl	12(%ebp), %edx	# get argumentCount
	movl	%eax, 64(%ebp,%edx,4)	# store result

/* return */
	ret

	.size	XFunction.0.__apply__.code, .-XFunction.0.__apply__.code


	.text
	.align	4
XFunction.0.unboxed:
	.globl	XFunction.0.unboxed
	.type	XFunction.0.unboxed, @object
	.size	XFunction.0.unboxed, 16
	.long	Method
	.long	0
	.long	0
	.long	0
XFunction.0.unboxed.code:
	.globl	XFunction.0.unboxed.code
	.type	XFunction.0.unboxed.code, @function
	movl	%esi, 64(%ebp)	# fake, safe result for Spike code
	movl	4(%edi), %eax	# real result for C/asm code
	movl	$4, %ecx	# result size
	ret
	.size	XFunction.0.unboxed.code, .-XFunction.0.unboxed.code
