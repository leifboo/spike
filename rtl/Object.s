
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

/* tally number of instance variables in %eax */
	movl	%esi, %edi 	# get class of new object (i.e., self)
	movl	$0, %eax 	# tally instance vars
.L2:
	addl	16(%edi), %eax	# instVarCount
	movl	4(%edi), %edi 	# up superclass chain
	testl	%edi, %edi
	jne	.L2

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
