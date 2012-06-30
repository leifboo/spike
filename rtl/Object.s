
/*------------------------------------------------------------------------*/
/* class */

	.text
	.align	4
Object.0.class:
	.globl	Object.0.class
	.type	Object.0.class, @object
	.size	Object.0.class, 16
	.long	Method
	.long	0
	.long	0
	.long	0
Object.0.class.code:
	.globl	Object.0.class.code
	.type	Object.0.class.code, @function
	call	SpikeGetClass
	movl	%ebx, 8(%ebp)
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret	$0
	.size	Object.0.class.code, .-Object.0.class.code


SpikeGetClass:
	.globl	SpikeGetClass
	.type	SpikeGetClass, @function

	cmpl	$0, %esi	# test for null
	jne	.L11

	movl	$Null, %ebx
	ret
.L11:
	movl	%esi, %eax
	andl	$3, %eax	# test for object pointer
	cmpl	$0, %eax
	jne	.L12

	movl	(%esi), %ebx 	# get class
	ret
.L12:
	cmpl	$2, %eax	# test for SmallInteger
	jne	.L13

	movl	$Integer, %ebx
	ret
.L13:
	cmpl	$3, %eax	# test for CObject (aligned pointer)
	jne	.L14

	movl	$CObject, %ebx
	ret
.L14:
	pushl	$__sym_badObjectPointer
	call	SpikeError
	movl	$0, %ebx
	ret

	.size	SpikeGetClass, .-SpikeGetClass


/*------------------------------------------------------------------------*/
/* basicNew: */

	.text
	.align	4
Object.class.0.basicNew$:
	.globl	Object.class.0.basicNew$
	.type	Object.class.0.basicNew$, @object
	.size	Object.class.0.basicNew$, 16
	.long	Method
	.long	1
	.long	1
	.long	0
Object.class.0.basicNew$.code:
	.globl	Object.class.0.basicNew$.code
	.type	Object.class.0.basicNew$.code, @function

/* check type of arg */
	movl	8(%ebp), %edx	# get arg
	movl	%edx, %eax
	andl	$3, %eax	# test for SmallInteger
	cmpl	$2, %eax
	je	.L2
.L1:
	pushl	$__sym_typeError
	call	SpikeError

.L2:
/* tally number of instance variables in %eax */
	movl	%esi, %edi 	# get class of new object (i.e., self)
	movl	$0, %eax 	# tally instance vars
.L3:
	addl	16(%edi), %eax	# instVarCount
	movl	4(%edi), %edi 	# up superclass chain
	testl	%edi, %edi
	jne	.L3

/* add the requested number of indexable variables */
	sarl	$2, %edx
	addl	%edx, %eax

/* account for 'klass' is-a pointer */
	incl	%eax

/* allocate memory (assume an infinite amount memory for now!) */
	pushl	$4		# element size
	pushl	%eax		# number of elements
	call	calloc
	addl	$8, %esp

/* initialize 'klass' is-a pointer */
	movl	%esi, (%eax)
	
/* store result */
	movl	%eax, 12(%ebp)

/* clean up */
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Object.class.0.basicNew$.code, .-Object.class.0.basicNew$.code


/*------------------------------------------------------------------------*/
/* boxing/unboxing */

	.text
	.align	4
Object.class.0.box$:
	.globl	Object.class.0.box$
	.type	Object.class.0.box$, @object
	.size	Object.class.0.box$, 16
	.long	Method
	.long	1
	.long	1
	.long	0
Object.class.0.box$.code:
	.globl	Object.class.0.box$.code
	.type	Object.class.0.box$.code, @function
	movl	8(%ebp), %eax	# get pointer arg
	movl	%eax, 12(%ebp)	# provisional result
	andl	$3, %eax	# test for CObject
	cmpl	$3, %eax
	jne	.L4
	movl	8(%ebp), %eax	# get pointer arg
	andl	$~3, %eax	# discard flags
	movl	(%eax), %eax	# get result
	movl	%eax, 12(%ebp)	# store result
.L4:
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$4
	.size	Object.class.0.box$.code, .-Object.class.0.box$.code


	.text
	.align	4
Object.0.unboxed:
	.globl	Object.0.unboxed
	.type	Object.0.unboxed, @object
	.size	Object.0.unboxed, 16
	.long	Method
	.long	0
	.long	0
	.long	0
Object.0.unboxed.code:
	.globl	Object.0.unboxed.code
	.type	Object.0.unboxed.code, @function
	movl	%esi, %eax	# real result is object pointer
	movl	%eax, 8(%ebp)	# fake result is the same
	movl	$4, %ecx	# result size
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret	$0
	.size	Object.0.unboxed.code, .-Object.0.unboxed.code

