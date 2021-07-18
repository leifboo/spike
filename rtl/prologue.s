
	.text

/*
 * SpikePrologue
 *
 * Entered from message send bottleneck upon successful lookup.
 * Function|__apply__ also jumps here.
 *
 * On entry:
 *
 *     %rax: undefined
 *     %rcx: undefined
 *     %rdx: argumentCount
 *
 *     %rbx: methodClass in context of new method
 *     %rsi: self in context of new method
 *     %rdi: instVarPointer for Method/Function object itself
 *
 *     %rbp: activeContext / sender
 *
 *     |------------|
 *     | receiver   | receiver / space for result
 *     |------------|
 *     | arg 1      | args pushed by caller left-to-right
 *     | arg 2      |
 *     | ...        |
 *     | arg N      |
 *     |------------| <- %rsp
 */

SpikePrologue:
	.globl	SpikePrologue
	.type	SpikePrologue, @function

	cmp	0(%rdi), %rdx	# minArgumentCount
	jb	wrongNumberOfArguments
	mov	8(%rdi), %rax	# maxArgumentCount
	cmp	%rax, %rdx
	ja	wrongNumberOfArguments

	and	$0x7FFFFFFF, %rax	# get maxFixedArgumentCount
	mov	%rax, %rcx
	sub	%rdx, %rcx		# calc missingFixedArgumentCount
	jbe	newMethodContext

.L2:
	push	$0		# allocate missing fixed args
	loop	.L2

/*
 * allocate and initialize the new MethodContext
 */

newMethodContext:

	mov	16(%rdi), %rcx	# localCount
	add	$16, %rcx	# add Context overhead
.L3:
	push	$0		# alloc and init locals and Context header
	loop	.L3

	cmp	%rdx, %rax	# calc max(maxFixedArgumentCount, argumentCount)
	ja	.L4
	mov	%rdx, %rax
.L4:
	add	16(%rdi), %rax	# add localCount
	mov	%rax, 120(%rsp)	# -> size

	movq	$__spk_x_MethodContext, (%rsp)	# -> klass is-a pointer
	mov	%rbp, 8(%rsp)	# -> caller
	mov	%rsp, 16(%rsp)	# -> homeContext

	lea	24(%rdi), %rax	# skip Method instance variables
	mov	%rax, 32(%rsp)	# -> pc
	lea	-8(%rdi), %rax	# skip klass pointer
	mov	%rax, 80(%rsp)	# -> method

/* set up receiver's instance variable pointer in %rdi */
	mov	%rbx, %rdi 	# get class
	mov	$0, %rax 	# tally instance vars...
	jmp	.L6 		# ...in superclasses
.L5:
	add	32(%rdi), %rax	# instVarCount
.L6:
	mov	8(%rdi), %rdi 	# up superclass chain
	test	%rdi, %rdi
	jne	.L5

	lea	8(%rsi,%rax,8), %rdi

/* save Context registers */
	mov	%rdx, 24(%rsp)	# -> argumentCount
	mov	%rbx, 88(%rsp)	# -> methodClass
	mov	%rsi, 96(%rsp)	# -> receiver
	mov	%rdi, 104(%rsp)	# -> instVarPointer

/* enable methods to return with a one-byte 'ret' instruction */
	push	$SpikeEpilogue

	mov	%rsp, 112+8(%rsp)	# -> stackBase

/*
 * call the method
 *
 * On entry:
 *
 *     %rax: undefined
 *     %rcx: undefined
 *     %rdx: undefined
 *
 *     %rbx: homeContext (== activeContext)
 *     %rsi: self
 *     %rdi: instVarPointer
 *
 *     %rbp: activeContext
 *     %rsp: stackBase
 */
	lea	8(%rsp), %rbp	# switch to new context
	mov	16(%rbp), %rbx
	mov	32(%rbp), %rax	# load initial pc
	jmp	*%rax		# jump to it

	.size	SpikePrologue, .-SpikePrologue


wrongNumberOfArguments:
	push	$__spk_sym_wrongNumberOfArguments
	call	SpikeError
