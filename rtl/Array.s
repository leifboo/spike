
	.text

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
	movl	4(%edi,%edx,4), %eax 	# get item
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
	movl	%eax, 4(%edi,%edx,4) 	# set item
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
	ret
	.size	typeRangeCheck, .-typeRangeCheck
