
	.text

/*
 * SpikePushVarArgs:  pop an array off the stack, replacing it with
 * its contents; set %ecx to the size of the array.  This is tricky
 * since all registers except %eax and %ecx must be preserved.
 */
SpikePushVarArgs:
	.globl	SpikePushVarArgs
	.type	SpikePushVarArgs, @function

	movl	4(%esp), %eax	# check arg type
	movl	(%eax), %eax
	cmpl	$Array, %eax
	je	.L8
	pushl	$__sym_typeError
	call	SpikeError
.L8:
	movl	4(%esp), %eax	# get array
	movl	4(%eax), %ecx	# get size
	cmpl	$0, %ecx
	jne	.L9

/* empty array */
	popl	%eax		# pop return address
	popl	%ecx		# pop array
	movl	$0, %ecx	# no items
	jmp	*%eax		# return
	
.L9:
	cmpl	$1, %ecx
	jne	.L10

/* 1 item array */
	popl	%eax		# pop return address
	popl	%ecx		# pop array
	pushl	8(%ecx)		# push item
	movl	$1, %ecx	# 1 item
	jmp	*%eax		# return

.L10:
	cmpl	$2, %ecx
	jne	.L11

/* 2 item array */
	popl	%eax		# pop return address
	popl	%ecx		# pop array
	pushl	8(%ecx)		# push items
	pushl	12(%ecx)
	movl	$2, %ecx	# 2 items
	jmp	*%eax		# return

.L11:
/* array has at least three items; push a[2:N] */
	movl	$2, %ecx
.L12:
	pushl	8(%eax,%ecx,4)
	incl	%ecx
	cmpl	4(%eax), %ecx
	jb	.L12

	movl	-8(%esp,%ecx,4),%eax	# get return address
	pushl	%eax		# save it
	pushl	%ebp		# we need another register

/*
 * Stack is now:
 *
 *   a, ra, a[2], a[3], ... a[N-1], ra, ebp
 *                                         ^ %esp
 *
 * Replace array and return address with a[0] and a[1],
 * then leave.
 */

	movl	4(%esp,%ecx,4),%ebp	# get array
	leal	8(%ebp),%ebp	# point to a[0]
	movl	(%ebp), %eax	# move a[0]
	movl	%eax, 4(%esp,%ecx,4)
	movl	4(%ebp), %eax	# move a[1]
	movl	%eax, (%esp,%ecx,4)
	popl	%ebp		# restore
	ret

	.size	SpikePushVarArgs, .-SpikePushVarArgs


	.align	4
Array.0.init$:
	.globl	Array.0.init$
	.type	Array.0.init$, @object
	.size	Array.0.init$, 16
	.long	Method
	.long	1
	.long	1
	.long	0
Array.0.init$.code:
	.globl	Array.0.init$.code
	.type	Array.0.init$.code, @function
	movl	8(%ebp), %edx	# get size arg
	movl	%edx, %eax
	andl	$3, %eax	# test for SmallInteger
	cmpl	$2, %eax
	je	.L1
	pushl	$__sym_typeError
	call	SpikeError
.L1:
	sarl	$2, %edx	# unbox size
	movl	%edx, 0(%edi)	# save it
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Array.0.init$.code, .-Array.0.init$.code

	.align	4
Array.0.initWithContentsOfStack$:
	.globl	Array.0.initWithContentsOfStack$
	.type	Array.0.initWithContentsOfStack$, @object
	.size	Array.0.initWithContentsOfStack$, 16
	.long	Method
	.long	1
	.long	1
	.long	0
Array.0.initWithContentsOfStack$.code:
	.globl	Array.0.initWithContentsOfStack$.code
	.type	Array.0.initWithContentsOfStack$.code, @function
	movl	8(%ebp), %edx	# get pointer arg
	movl	%edx, %eax
	andl	$3, %eax	# test for CObject
	cmpl	$3, %eax
	je	.L5
	pushl	$__sym_typeError
	call	SpikeError
.L5:
	andl	$~3, %edx	# unbox pointer
	movl	(%edi), %ecx	# get size
	addl	$4, %edi	# skip size
	cmpl	$0, %ecx
	je	.L7
.L6:
	movl	-4(%edx,%ecx,4), %eax 	# get items from stack in reverse
	movl	%eax, (%edi)	# init my items
	addl	$4, %edi	# next item
	loop	.L6
.L7:
	movl	%esi, 12(%ebp)	# return self
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Array.0.initWithContentsOfStack$.code, .-Array.0.initWithContentsOfStack$.code

	.align	4
Array.0.size:
	.globl	Array.0.size
	.type	Array.0.size, @object
	.size	Array.0.size, 4
	.long	Method
	.long	0
	.long	0
	.long	0
Array.0.size.code:
	.globl	Array.0.size.code
	.type	Array.0.size.code, @function
	movl	(%edi), %eax	# get size
	sall	$2, %eax	# box it
	orl	$2, %eax
	movl	%eax, 8(%ebp)	# return it
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	Array.0.size.code, .-Array.0.size.code

	.align	4
Array.0.__index__:
	.globl	Array.0.__index__
	.type	Array.0.__index__, @object
	.size	Array.0.__index__, 4
	.long	Method
	.long	1
	.long	1
	.long	0
Array.0.__index__.code:
	.globl	Array.0.__index__.code
	.type	Array.0.__index__.code, @function
	movl	8(%ebp), %edx		# get index arg
	call	typeRangeCheck		# check type & range
	movl	(%edi,%edx,4), %eax 	# get item
	movl	%eax, 12(%ebp)		# return it
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Array.0.__index__.code, .-Array.0.__index__.code

	.align	4
Array.1.__index__:
	.globl	Array.1.__index__
	.type	Array.1.__index__, @object
	.size	Array.1.__index__, 4
	.long	Method
	.long	2
	.long	2
	.long	0
Array.1.__index__.code:
	.globl	Array.1.__index__.code
	.type	Array.1.__index__.code, @function
	movl	12(%ebp), %edx		# get index arg
	call	typeRangeCheck		# check type & range
	movl	8(%ebp), %eax		# get item arg
	movl	%eax, (%edi,%edx,4) 	# set item
	movl	%esi, 16(%ebp)		# return self
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$8
	.size	Array.1.__index__.code, .-Array.1.__index__.code


typeRangeCheck:
	.type	typeRangeCheck, @function
	movl	%edx, %eax
	andl	$3, %eax	# test for SmallInteger
	cmpl	$2, %eax
	je	.L2
	pushl	$__sym_typeError
	call	SpikeError
.L2:
	sarl	$2, %edx	# unbox index arg
	cmpl	$0, %edx	# check 0 <= index
	js	.L3
	movl	(%edi), %eax	# check index < size
	cmpl	%edx, %eax
	ja	.L4
.L3:
	pushl	$__sym_rangeError
	call	SpikeError
.L4:
/* tally number of instance variables in %eax */
	movl	(%esi), %edi 	# get class of array (Array itself or a subclass)
	movl	$0, %eax 	# tally instance vars
.L13:
	addl	16(%edi), %eax	# instVarCount
	movl	4(%edi), %edi 	# up superclass chain
	testl	%edi, %edi
	jne	.L13
/* set up pointer to indexable variables in %edi */
	leal	4(%esi,%eax,4), %edi
	ret
	.size	typeRangeCheck, .-typeRangeCheck
