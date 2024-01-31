
	.text

/*
 * SpikePushVarArgs:  pop an array off the stack, replacing it with
 * its contents; set %rcx to the size of the array.  This is tricky
 * since all registers except %rax and %rcx must be preserved.
 */
SpikePushVarArgs:
	.globl	SpikePushVarArgs
	.type	SpikePushVarArgs, @function

	mov	8(%rsp), %rax	# check arg type
	mov	(%rax), %rax
	cmp	$__spk_x_Array, %rax
	je	.L8
	pushq	$__spk_sym_typeError
	call	SpikeError
.L8:
	mov	8(%rsp), %rax	# get array
	mov	8(%rax), %rcx	# get size
	cmp	$0, %rcx
	jne	.L9

/* empty array */
	pop	%rax		# pop return address
	pop	%rcx		# pop array
	mov	$0, %rcx	# no items
	jmp	*%rax		# return
	
.L9:
	cmp	$1, %rcx
	jne	.L10

/* 1 item array */
	pop	%rax		# pop return address
	pop	%rcx		# pop array
	push	16(%rcx)	# push item
	mov	$1, %rcx	# 1 item
	jmp	*%rax		# return

.L10:
	cmp	$2, %rcx
	jne	.L11

/* 2 item array */
	pop	%rax		# pop return address
	pop	%rcx		# pop array
	push	16(%rcx)	# push items
	push	24(%rcx)
	mov	$2, %rcx	# 2 items
	jmp	*%rax		# return

.L11:
/* array has at least three items; push a[2:N] */
	mov	$2, %rcx
.L12:
	push	16(%rax,%rcx,8)
	inc	%rcx
	cmp	8(%rax), %rcx
	jb	.L12

	mov	-16(%rsp,%rcx,8),%rax	# get return address
	push	%rax		# save it
	push	%rbp		# we need another register

/*
 * Stack is now:
 *
 *   a, ra, a[2], a[3], ... a[N-1], ra, rbp
 *                                         ^ %rsp
 *
 * Replace array and return address with a[0] and a[1],
 * then leave.
 */

	mov	8(%rsp,%rcx,8),%rbp	# get array
	lea	16(%rbp),%rbp	# point to a[0]
	mov	(%rbp), %rax	# move a[0]
	mov	%rax, 8(%rsp,%rcx,8)
	mov	8(%rbp), %rax	# move a[1]
	mov	%rax, (%rsp,%rcx,8)
	pop	%rbp		# restore
	ret

	.size	SpikePushVarArgs, .-SpikePushVarArgs


	.align	8
Array.0.init$:
	.globl	Array.0.init$
	.type	Array.0.init$, @object
	.size	Array.0.init$, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Array.0.init$.code:
	.globl	Array.0.init$.code
	.type	Array.0.init$.code, @function
	mov	128(%rbp), %rdx	# get size arg
	mov	%rdx, %rax
	and	$3, %rax	# test for SmallInteger
	cmp	$2, %rax
	je	.L1
	push	$__spk_sym_typeError
	call	SpikeError
.L1:
	sar	$2, %rdx	# unbox size
	mov	%rdx, 0(%rdi)	# save it
	ret
	.size	Array.0.init$.code, .-Array.0.init$.code

	.align	8
Array.0.size:
	.globl	Array.0.size
	.type	Array.0.size, @object
	.size	Array.0.size, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
Array.0.size.code:
	.globl	Array.0.size.code
	.type	Array.0.size.code, @function
	mov	(%rdi), %rax	# get size
	sal	$2, %rax	# box it
	or	$2, %rax
	mov	%rax, 128(%rbp)	# return it
	ret
	.size	Array.0.size.code, .-Array.0.size.code

	.align	8
Array.0.__index__:
	.globl	Array.0.__index__
	.type	Array.0.__index__, @object
	.size	Array.0.__index__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Array.0.__index__.code:
	.globl	Array.0.__index__.code
	.type	Array.0.__index__.code, @function
	mov	128(%rbp), %rdx		# get index arg
	call	typeRangeCheck		# check type & range
	mov	(%rdi,%rdx,8), %rax 	# get item
	mov	%rax, 136(%rbp)		# return it
	ret
	.size	Array.0.__index__.code, .-Array.0.__index__.code

	.align	8
Array.1.__index__:
	.globl	Array.1.__index__
	.type	Array.1.__index__, @object
	.size	Array.1.__index__, 32
	.quad	__spk_x_Method
	.quad	2
	.quad	2
	.quad	0
Array.1.__index__.code:
	.globl	Array.1.__index__.code
	.type	Array.1.__index__.code, @function
	mov	136(%rbp), %rdx		# get index arg
	call	typeRangeCheck		# check type & range
	mov	128(%rbp), %rax		# get item arg
	mov	%rax, (%rdi,%rdx,8) 	# set item
	mov	%rsi, 144(%rbp)		# return self
	ret
	.size	Array.1.__index__.code, .-Array.1.__index__.code


typeRangeCheck:
	.type	typeRangeCheck, @function
	mov	%rdx, %rax
	and	$3, %rax	# test for SmallInteger
	cmp	$2, %rax
	je	.L2
	push	$__spk_sym_typeError
	call	SpikeError
.L2:
	sar	$2, %rdx	# unbox index arg
	cmp	$0, %rdx	# check 0 <= index
	js	.L3
	mov	(%rdi), %rax	# check index < size
	cmp	%rdx, %rax
	ja	.L4
.L3:
	push	$__spk_sym_rangeError
	call	SpikeError
.L4:
/* tally number of instance variables in %rax */
	mov	(%rsi), %rdi 	# get class of array (Array itself or a subclass)
	mov	$0, %rax 	# tally instance vars
.L13:
	add	32(%rdi), %rax	# instVarCount
	mov	8(%rdi), %rdi 	# up superclass chain
	test	%rdi, %rdi
	jne	.L13
/* set up pointer to indexable variables in %rdi */
	lea	8(%rsi,%rax,8), %rdi
	ret
	.size	typeRangeCheck, .-typeRangeCheck
