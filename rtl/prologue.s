
	.text

/*
 * SpikePrologue
 *
 * Entered from message send bottleneck upon successful lookup.
 * Function|__apply__ also jumps here.
 *
 * On entry:
 *
 *     %eax: undefined
 *     %ecx: undefined
 *     %edx: argumentCount
 *
 *     %ebx: methodClass in context of new method
 *     %esi: self in context of new method
 *     %edi: instVarPointer for Method/Function object itself
 *
 *     %ebp: activeContext / sender
 *
 *     |------------|
 *     | receiver   | receiver / space for result
 *     |------------|
 *     | arg 1      | args pushed by caller left-to-right
 *     | arg 2      |
 *     | ...        |
 *     | arg N      |
 *     |------------| <- %esp
 */

SpikePrologue:
	.globl	SpikePrologue
	.type	SpikePrologue, @function

	cmpl	0(%edi), %edx	# minArgumentCount
	jb	wrongNumberOfArguments
	movl	4(%edi), %eax	# maxArgumentCount
	cmpl	%eax, %edx
	ja	wrongNumberOfArguments

	andl	$0x7FFFFFFF, %eax	# get maxFixedArgumentCount
	movl	%eax, %ecx
	subl	%edx, %ecx		# calc missingFixedArgumentCount
	jbe	newMethodContext

.L2:
	pushl	$0		# allocate missing fixed args
	loop	.L2

/*
 * allocate and initialize the new MethodContext
 */

newMethodContext:

	movl	8(%edi), %ecx	# localCount
	addl	$16, %ecx	# add Context overhead
.L3:
	pushl	$0		# alloc and init locals and Context header
	loop	.L3

	cmpl	%edx, %eax	# calc max(maxFixedArgumentCount, argumentCount)
	ja	.L4
	movl	%edx, %eax
.L4:
	addl	8(%edi), %eax	# add localCount
	movl	%eax, 60(%esp)	# -> size

	movl	$MethodContext, (%esp)	# -> klass is-a pointer
	movl	%ebp, 4(%esp)	# -> caller
	movl	%esp, 8(%esp)	# -> homeContext

	leal	12(%edi), %eax	# skip Method instance variables
	movl	%eax, 16(%esp)	# -> pc
	leal	-4(%edi), %eax	# skip klass pointer
	movl	%eax, 40(%esp)	# -> method

/* set up receiver's instance variable pointer in %edi */
	movl	%ebx, %edi 	# get class
	movl	$0, %eax 	# tally instance vars...
	jmp	.L6 		# ...in superclasses
.L5:
	addl	16(%edi), %eax	# instVarCount
.L6:
	movl	4(%edi), %edi 	# up superclass chain
	testl	%edi, %edi
	jne	.L5

	leal	4(%esi,%eax,4), %edi

/* save Context registers */
	movl	%edx, 12(%esp)	# -> argumentCount
	movl	%ebx, 44(%esp)	# -> methodClass
	movl	%esi, 48(%esp)	# -> receiver
	movl	%edi, 52(%esp)	# -> instVarPointer

/* enable methods to return with a one-byte 'ret' instruction */
	pushl	$SpikeEpilogue

	movl	%esp, 56+4(%esp)	# -> stackBase

/*
 * call the method
 *
 * On entry:
 *
 *     %eax: undefined
 *     %ecx: undefined
 *     %edx: undefined
 *
 *     %ebx: homeContext (== activeContext)
 *     %esi: self
 *     %edi: instVarPointer
 *
 *     %ebp: activeContext
 *     %esp: stackBase
 */
	leal	4(%esp), %ebp	# switch to new context
	movl	8(%ebp), %ebx
	movl	16(%ebp), %eax	# load initial pc
	jmpl	*%eax		# jump to it

	.size	SpikePrologue, .-SpikePrologue


wrongNumberOfArguments:
	pushl	$__sym_wrongNumberOfArguments
	call	SpikeError
