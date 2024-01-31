
	.text

main:
	.globl	main

	call	SpikeInstallTrapHandler

/* allocate and initialize the first Context */
	lea	-128(%rsp), %rsp
	mov	$0, %rax
	movq	$__spk_x_MethodContext, (%rsp)	# -> klass is-a pointer
	mov	%rax, 8(%rsp)	# 0 -> caller
	mov	%rsp, 16(%rsp)	# -> homeContext
	mov	%rax, 24(%rsp)	# 0 -> argumentCount
	mov	%rax, 32(%rsp)	# 0 -> pc
	mov	%rax, 40(%rsp)	# 0 -> sp
	mov	%rax, 48(%rsp)	# 0 -> regSaveArea[0]
	mov	%rax, 56(%rsp)	# 0 -> regSaveArea[1]
	mov	%rax, 64(%rsp)	# 0 -> regSaveArea[2]
	mov	%rax, 72(%rsp)	# 0 -> regSaveArea[3]
	mov	%rax, 80(%rsp)	# 0 -> method
	mov	%rax, 88(%rsp)	# 0 -> methodClass
	mov	%rax, 96(%rsp)	# 0 -> receiver
	mov	%rax, 104(%rsp)	# 0 -> instVarPointer
	mov	%rsp, 112(%rsp)	# -> stackBase
	mov	%rax, 120(%rsp)	# 0 -> size
	mov	%rsp, %rbp
	mov	%rbp, %rbx

	push	$__spk_x_main	# receiver / space for result
	push	$0		# XXX: args
	mov	$1, %rcx
	call	SpikeCall	# call 'main' object
	pop	%rax		# get result
	cmp	$__spk_x_void, %rax	# check for void
	je	.L2
	mov	%rax, %rdx
	and	$3, %rdx	# test for SmallInteger
	cmp	$2, %rdx
	je	.L1
	push	$__spk_sym_typeError
	call	SpikeError
.L1:
	sar	$2, %rax	# unbox result
	lea	128(%rsp), %rsp
	ret
.L2:
	mov	$0, %rax
	lea	128(%rsp), %rsp
	ret

