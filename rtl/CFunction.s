
	.text
	.align	8
CFunction.0.__apply__:
	.globl	CFunction.0.__apply__
	.type	CFunction.0.__apply__, @object
	.size	CFunction.0.__apply__, 32
	.quad	__spk_x_Method
	.quad	0		# minArgumentCount
	.quad	0x80000000	# maxArgumentCount
	.quad	0		# localCount
CFunction.0.__apply__.code:
	.globl	CFunction.0.__apply__.code
	.type	CFunction.0.__apply__.code, @function

/* pad the stack for our poor man's FFI */
	push	$0
	push	$0
	push	$0
	push	$0
	push	$0
	push	$0
	push	$0
/* copy, reverse, and unbox args */
	mov	$0, %rsi	# start at last arg
	mov	$6, %r12	# available SSE registers
	jmp	.L2
.L1:
	push	128(%rbp,%rsi,8)	# push arg
	mov	$__spk_sym_unboxed, %rdx
	call	SpikeGetAttr	# unbox arg
	cmp	$16, %rcx
	jne	.L3
	pop	%rdx		# real result in SSE register
	jmp	.L4
.L3:
	mov	%rax, (%rsp)	# replace fake obj result with real one
.L4:
	add	$1, %rsi	# walk back to previous arg
.L2:
	cmp	24(%rbp), %rsi	# compare with argumentCount
	jb	.L1

	cmp	$0, %r12
	je	.L6
	cmp	$6, %r12
	je	.L6
	mov	%r12, %rcx
/* shift SSE registers */
.L5:
	movsd	%xmm1, %xmm0
	movsd	%xmm2, %xmm1
	movsd	%xmm3, %xmm2
	movsd	%xmm4, %xmm3
	movsd	%xmm5, %xmm4
	loop	.L5
.L6:
/* call C function */
	mov	8(%rdi), %r10	# get function pointer
/* unfurl register args */
	pop	%rdi
	pop	%rsi
	pop	%rdx
	pop	%rcx
	pop	%r8
	pop	%r9
	and	$0xfffffffffffffff0, %rsp
	mov	$6, %rax
	call	*%r10		# call it
	mov	112(%rbp), %rsp	# clean up C args
	mov	104(%rbp), %rdi	# restore regs
	mov	96(%rbp), %rsi

/* box result */
	push	%rax		# arg is Spike pointer (CObject)
	mov	%rsp, %rax
	or	$3, %rax
	push	(%rdi)		# receiver = signature
	push	%rax		# arg
	mov	$__spk_sym_box$, %rdx
	mov	$1, %rcx	# argument count
	call	SpikeSendMessage
	mov	24(%rbp), %rdx	# get argumentCount
	pop	128(%rbp,%rdx,8)	# store result
	pop	%rax		# pop arg storage

/* return */
	ret

	.size	CFunction.0.__apply__.code, .-CFunction.0.__apply__.code


	.text
	.align	8
CFunction.0.unboxed:
	.globl	CFunction.0.unboxed
	.type	CFunction.0.unboxed, @object
	.size	CFunction.0.unboxed, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
CFunction.0.unboxed.code:
	.globl	CFunction.0.unboxed.code
	.type	CFunction.0.unboxed.code, @function
	mov	%rsi, 128(%rbp)	# fake, safe result for Spike code
	mov	8(%rdi), %rax	# real result for C/asm code
	mov	$8, %rcx	# result size
	ret
	.size	CFunction.0.unboxed.code, .-CFunction.0.unboxed.code
