
	.text
	.align	8
XFunction.0.__apply__:
	.globl	XFunction.0.__apply__
	.type	XFunction.0.__apply__, @object
	.size	XFunction.0.__apply__, 32
	.quad	__spk_x_Method
	.quad	0		# minArgumentCount
	.quad	0x80000000	# maxArgumentCount
	.quad	0		# localCount
XFunction.0.__apply__.code:
	.globl	XFunction.0.__apply__.code
	.type	XFunction.0.__apply__.code, @function

/* pad the stack for our poor man's FFI */
	push	$0
	push	$0
	push	$0
	push	$0
	push	$0
	push	$0
	push	$0
/* copy and reverse args */
	mov	$0, %rsi	# start at last arg
	jmp	.L2
.L1:
	push	128(%rbp,%rsi,8)	# push arg
	add	$1, %rsi	# walk back to previous arg
.L2:
	cmp	24(%rbp), %rsi	# compare with argumentCount
	jb	.L1

/* call C function */
	mov	8(%rdi), %rax	# get function pointer
/* unfurl register args */
	pop	%rdi
	pop	%rsi
	pop	%rdx
	pop	%rcx
	pop	%r8
	pop	%r9
	and	$0xfffffffffffffff0, %rsp
	call	*%rax		# call it
	mov	112(%rbp), %rsp	# clean up C args
	mov	104(%rbp), %rdi	# restore regs
	mov	96(%rbp), %rsi

/* store result */
	mov	24(%rbp), %rdx	# get argumentCount
	mov	%rax, 128(%rbp,%rdx,8)	# store result

/* return */
	ret

	.size	XFunction.0.__apply__.code, .-XFunction.0.__apply__.code


	.text
	.align	8
XFunction.0.unboxed:
	.globl	XFunction.0.unboxed
	.type	XFunction.0.unboxed, @object
	.size	XFunction.0.unboxed, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
XFunction.0.unboxed.code:
	.globl	XFunction.0.unboxed.code
	.type	XFunction.0.unboxed.code, @function
	mov	%rsi, 128(%rbp)	# fake, safe result for Spike code
	mov	8(%rdi), %rax	# real result for C/asm code
	mov	$8, %rcx	# result size
	ret
	.size	XFunction.0.unboxed.code, .-XFunction.0.unboxed.code
