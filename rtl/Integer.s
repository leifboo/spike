
/*------------------------------------------------------------------------*/
/*
 * unary operators
 */

	.text
	.align	4
Integer.0.__succ__:
	.globl	Integer.0.__succ__
	.type	Integer.0.__succ__, @object
	.size	Integer.0.__succ__, 4
	.long	Method
	.long	0
	.long	0
	.long	0
Integer.0.__succ__.code:
	.globl	Integer.0.__succ__.code
	.type	Integer.0.__succ__.code, @function
	addl	$4, %esi
	movl	%esi, 8(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	Integer.0.__succ__.code, .-Integer.0.__succ__.code

	.text
	.align	4
Integer.0.__pred__:
	.globl	Integer.0.__pred__
	.type	Integer.0.__pred__, @object
	.size	Integer.0.__pred__, 4
	.long	Method
	.long	0
	.long	0
	.long	0
Integer.0.__pred__.code:
	.globl	Integer.0.__pred__.code
	.type	Integer.0.__pred__.code, @function
	subl	$4, %esi
	movl	%esi, 8(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	Integer.0.__pred__.code, .-Integer.0.__pred__.code

	.text
	.align	4
Integer.0.__pos__:
	.globl	Integer.0.__pos__
	.type	Integer.0.__pos__, @object
	.size	Integer.0.__pos__, 4
	.long	Method
	.long	0
	.long	0
	.long	0
Integer.0.__pos__.code:
	.globl	Integer.0.__pos__.code
	.type	Integer.0.__pos__.code, @function
	movl	%esi, 8(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	Integer.0.__pos__.code, .-Integer.0.__pos__.code

	.text
	.align	4
Integer.0.__neg__:
	.globl	Integer.0.__neg__
	.type	Integer.0.__neg__, @object
	.size	Integer.0.__neg__, 4
	.long	Method
	.long	0
	.long	0
	.long	0
Integer.0.__neg__.code:
	.globl	Integer.0.__neg__.code
	.type	Integer.0.__neg__.code, @function
	xorl	$~3, %esi
	addl	$1, %esi
	movl	%esi, 8(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	Integer.0.__neg__.code, .-Integer.0.__neg__.code

	.text
	.align	4
Integer.0.__bneg__:
	.globl	Integer.0.__bneg__
	.type	Integer.0.__bneg__, @object
	.size	Integer.0.__bneg__, 4
	.long	Method
	.long	0
	.long	0
	.long	0
Integer.0.__bneg__.code:
	.globl	Integer.0.__bneg__.code
	.type	Integer.0.__bneg__.code, @function
	xorl	$~3, %esi
	movl	%esi, 8(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	Integer.0.__bneg__.code, .-Integer.0.__bneg__.code


/*------------------------------------------------------------------------*/
/*
 * binary operators
 */


binaryOperInit:
	.type	binaryOperInit, @function
	movl	8(%ebp), %eax	# get arg
	movl	%eax, %edx
	andl	$3, %edx	# test for SmallInteger
	cmpl	$2, %edx
	je	.L1
	pushl	$__sym_typeError
	call	SpikeError
.L1:
	sarl	$2, %esi	# unbox self
	sarl	$2, %eax	# unbox arg
	ret
	.size	binaryOperInit, .-binaryOperInit


	.text
	.align	4
Integer.0.__mul__:
	.globl	Integer.0.__mul__
	.type	Integer.0.__mul__, @object
	.size	Integer.0.__mul__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__mul__.code:
	.globl	Integer.0.__mul__.code
	.type	Integer.0.__mul__.code, @function
	call	binaryOperInit
	imull	%esi, %eax
	sall	$2, %eax
	orl	$2, %eax
	movl	%eax, 12(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__mul__.code, .-Integer.0.__mul__.code

	.text
	.align	4
Integer.0.__div__:
	.globl	Integer.0.__div__
	.type	Integer.0.__div__, @object
	.size	Integer.0.__div__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__div__.code:
	.globl	Integer.0.__div__.code
	.type	Integer.0.__div__.code, @function
	call	binaryOperInit
	xchgl	%esi, %eax	# dividend in edx:eax, divisor in esi
	movl	%eax, %edx
	sarl	$31, %edx
	idivl	%esi
	sall	$2, %eax
	orl	$2, %eax
	movl	%eax, 12(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__div__.code, .-Integer.0.__div__.code

	.text
	.align	4
Integer.0.__mod__:
	.globl	Integer.0.__mod__
	.type	Integer.0.__mod__, @object
	.size	Integer.0.__mod__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__mod__.code:
	.globl	Integer.0.__mod__.code
	.type	Integer.0.__mod__.code, @function
	call	binaryOperInit
	xchgl	%esi, %eax	# dividend in edx:eax, divisor in esi
	movl	%eax, %edx
	sarl	$31, %edx
	idivl	%esi
	sall	$2, %edx
	orl	$2, %edx
	movl	%edx, 12(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__mod__.code, .-Integer.0.__mod__.code

	.text
	.align	4
Integer.0.__add__:
	.globl	Integer.0.__add__
	.type	Integer.0.__add__, @object
	.size	Integer.0.__add__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__add__.code:
	.globl	Integer.0.__add__.code
	.type	Integer.0.__add__.code, @function
	call	binaryOperInit
	addl	%esi, %eax
	sall	$2, %eax
	orl	$2, %eax
	movl	%eax, 12(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__add__.code, .-Integer.0.__add__.code

	.text
	.align	4
Integer.0.__sub__:
	.globl	Integer.0.__sub__
	.type	Integer.0.__sub__, @object
	.size	Integer.0.__sub__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__sub__.code:
	.globl	Integer.0.__sub__.code
	.type	Integer.0.__sub__.code, @function
	call	binaryOperInit
	subl	%eax, %esi
	sall	$2, %esi
	orl	$2, %esi
	movl	%esi, 12(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__sub__.code, .-Integer.0.__sub__.code

	.text
	.align	4
Integer.0.__lshift__:
	.globl	Integer.0.__lshift__
	.type	Integer.0.__lshift__, @object
	.size	Integer.0.__lshift__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__lshift__.code:
	.globl	Integer.0.__lshift__.code
	.type	Integer.0.__lshift__.code, @function
	call	binaryOperInit
	movl	%eax, %ecx
	sall	%cl, %esi
	sall	$2, %esi
	orl	$2, %esi
	movl	%esi, 12(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__lshift__.code, .-Integer.0.__lshift__.code

	.text
	.align	4
Integer.0.__rshift__:
	.globl	Integer.0.__rshift__
	.type	Integer.0.__rshift__, @object
	.size	Integer.0.__rshift__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__rshift__.code:
	.globl	Integer.0.__rshift__.code
	.type	Integer.0.__rshift__.code, @function
	call	binaryOperInit
	movl	%eax, %ecx
	sarl	%cl, %esi
	sall	$2, %esi
	orl	$2, %esi
	movl	%esi, 12(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__rshift__.code, .-Integer.0.__rshift__.code

	.text
	.align	4
Integer.0.__band__:
	.globl	Integer.0.__band__
	.type	Integer.0.__band__, @object
	.size	Integer.0.__band__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__band__.code:
	.globl	Integer.0.__band__.code
	.type	Integer.0.__band__.code, @function
	call	binaryOperInit
	andl	%esi, %eax
	sall	$2, %eax
	orl	$2, %eax
	movl	%eax, 12(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__band__.code, .-Integer.0.__band__.code

	.text
	.align	4
Integer.0.__bxor__:
	.globl	Integer.0.__bxor__
	.type	Integer.0.__bxor__, @object
	.size	Integer.0.__bxor__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__bxor__.code:
	.globl	Integer.0.__bxor__.code
	.type	Integer.0.__bxor__.code, @function
	call	binaryOperInit
	xorl	%esi, %eax
	sall	$2, %eax
	orl	$2, %eax
	movl	%eax, 12(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__bxor__.code, .-Integer.0.__bxor__.code

	.text
	.align	4
Integer.0.__bor__:
	.globl	Integer.0.__bor__
	.type	Integer.0.__bor__, @object
	.size	Integer.0.__bor__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__bor__.code:
	.globl	Integer.0.__bor__.code
	.type	Integer.0.__bor__.code, @function
	call	binaryOperInit
	orl	%esi, %eax
	sall	$2, %eax
	orl	$2, %eax
	movl	%eax, 12(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__bor__.code, .-Integer.0.__bor__.code


/*------------------------------------------------------------------------*/
/*
 * comparison operators
 */

	.text
	.align	4
Integer.0.__lt__:
	.globl	Integer.0.__lt__
	.type	Integer.0.__lt__, @object
	.size	Integer.0.__lt__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__lt__.code:
	.globl	Integer.0.__lt__.code
	.type	Integer.0.__lt__.code, @function
	call	binaryOperInit
	movl	$true, 12(%ebp)
	cmpl	%eax, %esi
	jl	.L2
	movl	$false, 12(%ebp)
.L2:
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__lt__.code, .-Integer.0.__lt__.code

	.text
	.align	4
Integer.0.__gt__:
	.globl	Integer.0.__gt__
	.type	Integer.0.__gt__, @object
	.size	Integer.0.__gt__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__gt__.code:
	.globl	Integer.0.__gt__.code
	.type	Integer.0.__gt__.code, @function
	call	binaryOperInit
	movl	$true, 12(%ebp)
	cmpl	%eax, %esi
	jg	.L3
	movl	$false, 12(%ebp)
.L3:
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__gt__.code, .-Integer.0.__gt__.code

	.text
	.align	4
Integer.0.__le__:
	.globl	Integer.0.__le__
	.type	Integer.0.__le__, @object
	.size	Integer.0.__le__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__le__.code:
	.globl	Integer.0.__le__.code
	.type	Integer.0.__le__.code, @function
	call	binaryOperInit
	movl	$true, 12(%ebp)
	cmpl	%eax, %esi
	jle	.L4
	movl	$false, 12(%ebp)
.L4:
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__le__.code, .-Integer.0.__le__.code

	.text
	.align	4
Integer.0.__ge__:
	.globl	Integer.0.__ge__
	.type	Integer.0.__ge__, @object
	.size	Integer.0.__ge__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__ge__.code:
	.globl	Integer.0.__ge__.code
	.type	Integer.0.__ge__.code, @function
	call	binaryOperInit
	movl	$true, 12(%ebp)
	cmpl	%eax, %esi
	jge	.L5
	movl	$false, 12(%ebp)
.L5:
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__ge__.code, .-Integer.0.__ge__.code

	.text
	.align	4
Integer.0.__eq__:
	.globl	Integer.0.__eq__
	.type	Integer.0.__eq__, @object
	.size	Integer.0.__eq__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__eq__.code:
	.globl	Integer.0.__eq__.code
	.type	Integer.0.__eq__.code, @function
	call	binaryOperInit
	movl	$true, 12(%ebp)
	cmpl	%eax, %esi
	je	.L6
	movl	$false, 12(%ebp)
.L6:
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__eq__.code, .-Integer.0.__eq__.code

	.text
	.align	4
Integer.0.__ne__:
	.globl	Integer.0.__ne__
	.type	Integer.0.__ne__, @object
	.size	Integer.0.__ne__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.0.__ne__.code:
	.globl	Integer.0.__ne__.code
	.type	Integer.0.__ne__.code, @function
	call	binaryOperInit
	movl	$true, 12(%ebp)
	cmpl	%eax, %esi
	jne	.L7
	movl	$false, 12(%ebp)
.L7:
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.0.__ne__.code, .-Integer.0.__ne__.code


/*------------------------------------------------------------------------*/
/*
 * boxing/unboxing
 */

	.text
	.align	4
Integer.class.0.box$:
	.globl	Integer.class.0.box$
	.type	Integer.class.0.box$, @object
	.size	Integer.class.0.box$, 16
	.long	Method
	.long	1
	.long	1
	.long	0
Integer.class.0.box$.code:
	.globl	Integer.class.0.box$.code
	.type	Integer.class.0.box$.code, @function
	movl	8(%ebp), %eax	# get pointer arg
	movl	%eax, 12(%ebp)	# provisional result
	andl	$3, %eax	# test for CObject
	cmpl	$3, %eax
	jne	.L8
	movl	8(%ebp), %eax	# get pointer arg
	andl	$~3, %eax	# discard flags
	movl	(%eax), %eax	# get result
	sall	$2, %eax	# box it
	orl	$2, %eax
	movl	%eax, 12(%ebp)	# store result
.L8:
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Integer.class.0.box$.code, .-Integer.class.0.box$.code


	.text
	.align	4
Integer.0.unboxed:
	.globl	Integer.0.unboxed
	.type	Integer.0.unboxed, @object
	.size	Integer.0.unboxed, 16
	.long	Method
	.long	0
	.long	0
	.long	0
Integer.0.unboxed.code:
	.globl	Integer.0.unboxed.code
	.type	Integer.0.unboxed.code, @function
	movl	%esi, 8(%ebp)	# fake, safe result for Spike code is 'self'
	movl	%esi, %eax	# real result for C/asm code...
	sarl	$2, %eax	# ...is also 'self', but unboxed
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	Integer.0.unboxed.code, .-Integer.0.unboxed.code
