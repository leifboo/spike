
/*------------------------------------------------------------------------*/
/*
 * unary operators
 */

	.text
	.align	8
Integer.0.__succ__:
	.globl	Integer.0.__succ__
	.type	Integer.0.__succ__, @object
	.size	Integer.0.__succ__, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
Integer.0.__succ__.code:
	.globl	Integer.0.__succ__.code
	.type	Integer.0.__succ__.code, @function
	add	$4, %rsi
	mov	%rsi, 128(%rbp)
	ret
	.size	Integer.0.__succ__.code, .-Integer.0.__succ__.code

	.text
	.align	8
Integer.0.__pred__:
	.globl	Integer.0.__pred__
	.type	Integer.0.__pred__, @object
	.size	Integer.0.__pred__, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
Integer.0.__pred__.code:
	.globl	Integer.0.__pred__.code
	.type	Integer.0.__pred__.code, @function
	sub	$4, %rsi
	mov	%rsi, 128(%rbp)
	ret
	.size	Integer.0.__pred__.code, .-Integer.0.__pred__.code

	.text
	.align	8
Integer.0.__pos__:
	.globl	Integer.0.__pos__
	.type	Integer.0.__pos__, @object
	.size	Integer.0.__pos__, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
Integer.0.__pos__.code:
	.globl	Integer.0.__pos__.code
	.type	Integer.0.__pos__.code, @function
	mov	%rsi, 128(%rbp)
	ret
	.size	Integer.0.__pos__.code, .-Integer.0.__pos__.code

	.text
	.align	8
Integer.0.__neg__:
	.globl	Integer.0.__neg__
	.type	Integer.0.__neg__, @object
	.size	Integer.0.__neg__, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
Integer.0.__neg__.code:
	.globl	Integer.0.__neg__.code
	.type	Integer.0.__neg__.code, @function
	xor	$~3, %rsi
	add	$4, %rsi
	mov	%rsi, 128(%rbp)
	ret
	.size	Integer.0.__neg__.code, .-Integer.0.__neg__.code

	.text
	.align	8
Integer.0.__bneg__:
	.globl	Integer.0.__bneg__
	.type	Integer.0.__bneg__, @object
	.size	Integer.0.__bneg__, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
Integer.0.__bneg__.code:
	.globl	Integer.0.__bneg__.code
	.type	Integer.0.__bneg__.code, @function
	xor	$~3, %rsi
	mov	%rsi, 128(%rbp)
	ret
	.size	Integer.0.__bneg__.code, .-Integer.0.__bneg__.code


/*------------------------------------------------------------------------*/
/*
 * binary operators
 */


binaryOperInit:
	.type	binaryOperInit, @function
	mov	128(%rbp), %rax	# get arg
	mov	%rax, %rdx
	and	$3, %rdx	# test for SmallInteger
	cmp	$2, %rdx
	je	.L1
	push	$__spk_sym_typeError
	call	SpikeError
.L1:
	sar	$2, %rsi	# unbox self
	sar	$2, %rax	# unbox arg
	ret
	.size	binaryOperInit, .-binaryOperInit


	.text
	.align	8
Integer.0.__mul__:
	.globl	Integer.0.__mul__
	.type	Integer.0.__mul__, @object
	.size	Integer.0.__mul__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__mul__.code:
	.globl	Integer.0.__mul__.code
	.type	Integer.0.__mul__.code, @function
	call	binaryOperInit
	imul	%rsi, %rax
	sal	$2, %rax
	or	$2, %rax
	mov	%rax, 136(%rbp)
	ret
	.size	Integer.0.__mul__.code, .-Integer.0.__mul__.code

	.text
	.align	8
Integer.0.__div__:
	.globl	Integer.0.__div__
	.type	Integer.0.__div__, @object
	.size	Integer.0.__div__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__div__.code:
	.globl	Integer.0.__div__.code
	.type	Integer.0.__div__.code, @function
	call	binaryOperInit
	xchg	%rsi, %rax	# dividend in edx:eax, divisor in esi
	mov	%rax, %rdx
	sar	$31, %rdx
	idiv	%rsi
	sal	$2, %rax
	or	$2, %rax
	mov	%rax, 136(%rbp)
	ret
	.size	Integer.0.__div__.code, .-Integer.0.__div__.code

	.text
	.align	8
Integer.0.__mod__:
	.globl	Integer.0.__mod__
	.type	Integer.0.__mod__, @object
	.size	Integer.0.__mod__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__mod__.code:
	.globl	Integer.0.__mod__.code
	.type	Integer.0.__mod__.code, @function
	call	binaryOperInit
	xchg	%rsi, %rax	# dividend in edx:eax, divisor in esi
	mov	%rax, %rdx
	sar	$31, %rdx
	idiv	%rsi
	sal	$2, %rdx
	or	$2, %rdx
	mov	%rdx, 136(%rbp)
	ret
	.size	Integer.0.__mod__.code, .-Integer.0.__mod__.code

	.text
	.align	8
Integer.0.__add__:
	.globl	Integer.0.__add__
	.type	Integer.0.__add__, @object
	.size	Integer.0.__add__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__add__.code:
	.globl	Integer.0.__add__.code
	.type	Integer.0.__add__.code, @function
	call	binaryOperInit
	add	%rsi, %rax
	sal	$2, %rax
	or	$2, %rax
	mov	%rax, 136(%rbp)
	ret
	.size	Integer.0.__add__.code, .-Integer.0.__add__.code

	.text
	.align	8
Integer.0.__sub__:
	.globl	Integer.0.__sub__
	.type	Integer.0.__sub__, @object
	.size	Integer.0.__sub__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__sub__.code:
	.globl	Integer.0.__sub__.code
	.type	Integer.0.__sub__.code, @function
	call	binaryOperInit
	sub	%rax, %rsi
	sal	$2, %rsi
	or	$2, %rsi
	mov	%rsi, 136(%rbp)
	ret
	.size	Integer.0.__sub__.code, .-Integer.0.__sub__.code

	.text
	.align	8
Integer.0.__lshift__:
	.globl	Integer.0.__lshift__
	.type	Integer.0.__lshift__, @object
	.size	Integer.0.__lshift__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__lshift__.code:
	.globl	Integer.0.__lshift__.code
	.type	Integer.0.__lshift__.code, @function
	call	binaryOperInit
	mov	%rax, %rcx
	sal	%cl, %rsi
	sal	$2, %rsi
	or	$2, %rsi
	mov	%rsi, 136(%rbp)
	ret
	.size	Integer.0.__lshift__.code, .-Integer.0.__lshift__.code

	.text
	.align	8
Integer.0.__rshift__:
	.globl	Integer.0.__rshift__
	.type	Integer.0.__rshift__, @object
	.size	Integer.0.__rshift__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__rshift__.code:
	.globl	Integer.0.__rshift__.code
	.type	Integer.0.__rshift__.code, @function
	call	binaryOperInit
	mov	%rax, %rcx
	sar	%cl, %rsi
	sal	$2, %rsi
	or	$2, %rsi
	mov	%rsi, 136(%rbp)
	ret
	.size	Integer.0.__rshift__.code, .-Integer.0.__rshift__.code

	.text
	.align	8
Integer.0.__band__:
	.globl	Integer.0.__band__
	.type	Integer.0.__band__, @object
	.size	Integer.0.__band__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__band__.code:
	.globl	Integer.0.__band__.code
	.type	Integer.0.__band__.code, @function
	call	binaryOperInit
	and	%rsi, %rax
	sal	$2, %rax
	or	$2, %rax
	mov	%rax, 136(%rbp)
	ret
	.size	Integer.0.__band__.code, .-Integer.0.__band__.code

	.text
	.align	8
Integer.0.__bxor__:
	.globl	Integer.0.__bxor__
	.type	Integer.0.__bxor__, @object
	.size	Integer.0.__bxor__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__bxor__.code:
	.globl	Integer.0.__bxor__.code
	.type	Integer.0.__bxor__.code, @function
	call	binaryOperInit
	xor	%rsi, %rax
	sal	$2, %rax
	or	$2, %rax
	mov	%rax, 136(%rbp)
	ret
	.size	Integer.0.__bxor__.code, .-Integer.0.__bxor__.code

	.text
	.align	8
Integer.0.__bor__:
	.globl	Integer.0.__bor__
	.type	Integer.0.__bor__, @object
	.size	Integer.0.__bor__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__bor__.code:
	.globl	Integer.0.__bor__.code
	.type	Integer.0.__bor__.code, @function
	call	binaryOperInit
	or	%rsi, %rax
	sal	$2, %rax
	or	$2, %rax
	mov	%rax, 136(%rbp)
	ret
	.size	Integer.0.__bor__.code, .-Integer.0.__bor__.code


/*------------------------------------------------------------------------*/
/*
 * comparison operators
 */

	.text
	.align	8
Integer.0.__lt__:
	.globl	Integer.0.__lt__
	.type	Integer.0.__lt__, @object
	.size	Integer.0.__lt__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__lt__.code:
	.globl	Integer.0.__lt__.code
	.type	Integer.0.__lt__.code, @function
	call	binaryOperInit
	movq	$__spk_x_true, 136(%rbp)
	cmp	%rax, %rsi
	jl	.L2
	movq	$__spk_x_false, 136(%rbp)
.L2:
	ret
	.size	Integer.0.__lt__.code, .-Integer.0.__lt__.code

	.text
	.align	8
Integer.0.__gt__:
	.globl	Integer.0.__gt__
	.type	Integer.0.__gt__, @object
	.size	Integer.0.__gt__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__gt__.code:
	.globl	Integer.0.__gt__.code
	.type	Integer.0.__gt__.code, @function
	call	binaryOperInit
	movq	$__spk_x_true, 136(%rbp)
	cmp	%rax, %rsi
	jg	.L3
	movq	$__spk_x_false, 136(%rbp)
.L3:
	ret
	.size	Integer.0.__gt__.code, .-Integer.0.__gt__.code

	.text
	.align	8
Integer.0.__le__:
	.globl	Integer.0.__le__
	.type	Integer.0.__le__, @object
	.size	Integer.0.__le__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__le__.code:
	.globl	Integer.0.__le__.code
	.type	Integer.0.__le__.code, @function
	call	binaryOperInit
	movq	$__spk_x_true, 136(%rbp)
	cmp	%rax, %rsi
	jle	.L4
	movq	$__spk_x_false, 136(%rbp)
.L4:
	ret
	.size	Integer.0.__le__.code, .-Integer.0.__le__.code

	.text
	.align	8
Integer.0.__ge__:
	.globl	Integer.0.__ge__
	.type	Integer.0.__ge__, @object
	.size	Integer.0.__ge__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__ge__.code:
	.globl	Integer.0.__ge__.code
	.type	Integer.0.__ge__.code, @function
	call	binaryOperInit
	movq	$__spk_x_true, 136(%rbp)
	cmp	%rax, %rsi
	jge	.L5
	movq	$__spk_x_false, 136(%rbp)
.L5:
	ret
	.size	Integer.0.__ge__.code, .-Integer.0.__ge__.code

	.text
	.align	8
Integer.0.__eq__:
	.globl	Integer.0.__eq__
	.type	Integer.0.__eq__, @object
	.size	Integer.0.__eq__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__eq__.code:
	.globl	Integer.0.__eq__.code
	.type	Integer.0.__eq__.code, @function
	call	binaryOperInit
	movq	$__spk_x_true, 136(%rbp)
	cmp	%rax, %rsi
	je	.L6
	movq	$__spk_x_false, 136(%rbp)
.L6:
	ret
	.size	Integer.0.__eq__.code, .-Integer.0.__eq__.code

	.text
	.align	8
Integer.0.__ne__:
	.globl	Integer.0.__ne__
	.type	Integer.0.__ne__, @object
	.size	Integer.0.__ne__, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.0.__ne__.code:
	.globl	Integer.0.__ne__.code
	.type	Integer.0.__ne__.code, @function
	call	binaryOperInit
	movq	$__spk_x_true, 136(%rbp)
	cmp	%rax, %rsi
	jne	.L7
	movq	$__spk_x_false, 136(%rbp)
.L7:
	ret
	.size	Integer.0.__ne__.code, .-Integer.0.__ne__.code


/*------------------------------------------------------------------------*/
/*
 * boxing/unboxing
 */

	.text
	.align	8
Integer.class.0.box$:
	.globl	Integer.class.0.box$
	.type	Integer.class.0.box$, @object
	.size	Integer.class.0.box$, 32
	.quad	__spk_x_Method
	.quad	1
	.quad	1
	.quad	0
Integer.class.0.box$.code:
	.globl	Integer.class.0.box$.code
	.type	Integer.class.0.box$.code, @function
	mov	128(%rbp), %rax	# get pointer arg
	mov	%rax, 136(%rbp)	# provisional result
	and	$3, %rax	# test for CObject
	cmp	$3, %rax
	jne	.L8
	mov	128(%rbp), %rax	# get pointer arg
	and	$~3, %rax	# discard flags
	mov	(%rax), %rax	# get result
	sal	$2, %rax	# box it
	or	$2, %rax
	mov	%rax, 136(%rbp)	# store result
.L8:
	ret
	.size	Integer.class.0.box$.code, .-Integer.class.0.box$.code


	.text
	.align	8
Integer.0.unboxed:
	.globl	Integer.0.unboxed
	.type	Integer.0.unboxed, @object
	.size	Integer.0.unboxed, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
Integer.0.unboxed.code:
	.globl	Integer.0.unboxed.code
	.type	Integer.0.unboxed.code, @function
	mov	%rsi, 128(%rbp)	# fake, safe result for Spike code is 'self'
	mov	%rsi, %rax	# real result for C/asm code...
	sar	$2, %rax	# ...is also 'self', but unboxed
	mov	$8, %rcx	# result size
	ret
	.size	Integer.0.unboxed.code, .-Integer.0.unboxed.code
