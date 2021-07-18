
/*------------------------------------------------------------------------*/
/* class */

	.text
	.align	8
Object.0.class:
	.globl	Object.0.class
	.type	Object.0.class, @object
	.size	Object.0.class, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
Object.0.class.code:
	.globl	Object.0.class.code
	.type	Object.0.class.code, @function
	call	SpikeGetClass
	mov	%rbx, 128(%rbp)
	ret
	.size	Object.0.class.code, .-Object.0.class.code


SpikeGetClass:
	.globl	SpikeGetClass
	.type	SpikeGetClass, @function

	cmp	$0, %rsi	# test for null
	jne	.L11

	mov	$__spk_x_Null, %rbx
	ret
.L11:
	mov	%rsi, %rax
	and	$3, %rax	# test for object pointer
	cmp	$0, %rax
	jne	.L12

	mov	(%rsi), %rbx 	# get class
	ret
.L12:
	cmp	$2, %rax	# test for SmallInteger
	jne	.L13

	mov	$__spk_x_Integer, %rbx
	ret
.L13:
	cmp	$3, %rax	# test for CObject (aligned pointer)
	jne	.L14

	mov	$__spk_x_CObject, %rbx
	ret
.L14:
	push	$__spk_sym_badObjectPointer
	call	SpikeError
	mov	$0, %rbx
	ret

	.size	SpikeGetClass, .-SpikeGetClass


/*------------------------------------------------------------------------*/
/* basicNew: */

	.text
	.align	8
Object.class.0.basicNew$:
	.globl	Object.class.0.basicNew$
	.type	Object.class.0.basicNew$, @object
	.size	Object.class.0.basicNew$, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Object.class.0.basicNew$.code:
	.globl	Object.class.0.basicNew$.code
	.type	Object.class.0.basicNew$.code, @function

/* check type of arg */
	mov	128(%rbp), %rdx	# get arg
	mov	%rdx, %rax
	and	$3, %rax	# test for SmallInteger
	cmp	$2, %rax
	je	.L2
.L1:
	push	$__spk_sym_typeError
	call	SpikeError

.L2:
/* tally number of instance variables in %rax */
	mov	%rsi, %rdi 	# get class of new object (i.e., self)
	mov	$0, %rax 	# tally instance vars
.L3:
	add	32(%rdi), %rax	# instVarCount
	mov	8(%rdi), %rdi 	# up superclass chain
	test	%rdi, %rdi
	jne	.L3

/* add the requested number of indexable variables */
	sar	$2, %rdx
	add	%rdx, %rax

/* account for 'klass' is-a pointer */
	inc	%rax

/* allocate memory (assume an infinite amount memory for now!) */
	push	$8		# element size
	push	%rax		# number of elements
	pop	%rdi
	pop	%rsi
	call	calloc
	mov	104(%rbp), %rdi	# restore regs
	mov	96(%rbp), %rsi

/* initialize 'klass' is-a pointer */
	mov	%rsi, (%rax)
	
/* store result */
	mov	%rax, 136(%rbp)

/* return */
	ret

	.size	Object.class.0.basicNew$.code, .-Object.class.0.basicNew$.code


/*------------------------------------------------------------------------*/
/* boxing/unboxing */

	.text
	.align	8
Object.class.0.box$:
	.globl	Object.class.0.box$
	.type	Object.class.0.box$, @object
	.size	Object.class.0.box$, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Object.class.0.box$.code:
	.globl	Object.class.0.box$.code
	.type	Object.class.0.box$.code, @function
	mov	128(%rbp), %rax	# get pointer arg
	mov	%rax, 136(%rbp)	# provisional result
	and	$3, %rax	# test for CObject
	cmp	$3, %rax
	jne	.L4
	mov	128(%rbp), %rax	# get pointer arg
	and	$~3, %rax	# discard flags
	mov	(%rax), %rax	# get result
	mov	%rax, 136(%rbp)	# store result
.L4:
	ret
	.size	Object.class.0.box$.code, .-Object.class.0.box$.code


	.text
	.align	8
Object.0.unboxed:
	.globl	Object.0.unboxed
	.type	Object.0.unboxed, @object
	.size	Object.0.unboxed, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
Object.0.unboxed.code:
	.globl	Object.0.unboxed.code
	.type	Object.0.unboxed.code, @function
	mov	%rsi, %rax	# real result is object pointer
	mov	%rax, 128(%rbp)	# fake result is the same
	mov	$8, %rcx	# result size
	ret
	.size	Object.0.unboxed.code, .-Object.0.unboxed.code

