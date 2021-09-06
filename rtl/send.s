
	.text

/*** "x++", "++x" ***/

SpikeOperSucc:
	.globl	SpikeOperSucc
	mov	$__spk_sym___succ__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperSuccSuper:
	.globl	SpikeOperSuccSuper
	mov	$__spk_sym___succ__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x--", "--x" ***/

SpikeOperPred:
	.globl	SpikeOperPred
	mov	$__spk_sym___pred__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperPredSuper:
	.globl	SpikeOperPredSuper
	mov	$__spk_sym___pred__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "&x" ***/

SpikeOperAddr:
	.globl	SpikeOperAddr
	mov	$__spk_sym___addr__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperAddrSuper:
	.globl	SpikeOperAddrSuper
	mov	$__spk_sym___addr__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "+x" ***/

SpikeOperPos:
	.globl	SpikeOperPos
	mov	$__spk_sym___pos__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperPosSuper:
	.globl	SpikeOperPosSuper
	mov	$__spk_sym___pos__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "-x" ***/

SpikeOperNeg:
	.globl	SpikeOperNeg
	mov	$__spk_sym___neg__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperNegSuper:
	.globl	SpikeOperNegSuper
	mov	$__spk_sym___neg__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "~x" ***/

SpikeOperBNeg:
	.globl	SpikeOperBNeg
	mov	$__spk_sym___bneg__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperBNegSuper:
	.globl	SpikeOperBNegSuper
	mov	$__spk_sym___bneg__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "!x" ***/

SpikeOperLNeg:
	.globl	SpikeOperLNeg
	mov	$__spk_sym___lneg__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperLNegSuper:
	.globl	SpikeOperLNegSuper
	mov	$__spk_sym___lneg__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x * y" ***/

SpikeOperMul:
	.globl	SpikeOperMul
	mov	$__spk_sym___mul__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperMulSuper:
	.globl	SpikeOperMulSuper
	mov	$__spk_sym___mul__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x / y" ***/

SpikeOperDiv:
	.globl	SpikeOperDiv
	mov	$__spk_sym___div__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperDivSuper:
	.globl	SpikeOperDivSuper
	mov	$__spk_sym___div__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x % y" ***/

SpikeOperMod:
	.globl	SpikeOperMod
	mov	$__spk_sym___mod__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperModSuper:
	.globl	SpikeOperModSuper
	mov	$__spk_sym___mod__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x + y" ***/

SpikeOperAdd:
	.globl	SpikeOperAdd
	mov	$__spk_sym___add__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperAddSuper:
	.globl	SpikeOperAddSuper
	mov	$__spk_sym___add__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x - y" ***/

SpikeOperSub:
	.globl	SpikeOperSub
	mov	$__spk_sym___sub__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperSubSuper:
	.globl	SpikeOperSubSuper
	mov	$__spk_sym___sub__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x << y" ***/

SpikeOperLShift:
	.globl	SpikeOperLShift
	mov	$__spk_sym___lshift__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperLShiftSuper:
	.globl	SpikeOperLShiftSuper
	mov	$__spk_sym___lshift__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x >> y" ***/

SpikeOperRShift:
	.globl	SpikeOperRShift
	mov	$__spk_sym___rshift__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperRShiftSuper:
	.globl	SpikeOperRShiftSuper
	mov	$__spk_sym___rshift__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x < y" ***/

SpikeOperLT:
	.globl	SpikeOperLT
	mov	$__spk_sym___lt__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperLTSuper:
	.globl	SpikeOperLTSuper
	mov	$__spk_sym___lt__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x > y" ***/

SpikeOperGT:
	.globl	SpikeOperGT
	mov	$__spk_sym___gt__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperGTSuper:
	.globl	SpikeOperGTSuper
	mov	$__spk_sym___gt__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x <= y" ***/

SpikeOperLE:
	.globl	SpikeOperLE
	mov	$__spk_sym___le__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperLESuper:
	.globl	SpikeOperLESuper
	mov	$__spk_sym___le__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x >= y" ***/

SpikeOperGE:
	.globl	SpikeOperGE
	mov	$__spk_sym___ge__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperGESuper:
	.globl	SpikeOperGESuper
	mov	$__spk_sym___ge__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x == y" ***/

SpikeOperEq:
	.globl	SpikeOperEq
	mov	$__spk_sym___eq__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperEqSuper:
	.globl	SpikeOperEqSuper
	mov	$__spk_sym___eq__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x != y" ***/

SpikeOperNE:
	.globl	SpikeOperNE
	mov	$__spk_sym___ne__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperNESuper:
	.globl	SpikeOperNESuper
	mov	$__spk_sym___ne__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x & y" ***/

SpikeOperBAnd:
	.globl	SpikeOperBAnd
	mov	$__spk_sym___band__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperBAndSuper:
	.globl	SpikeOperBAndSuper
	mov	$__spk_sym___band__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x ^ y" ***/

SpikeOperBXOr:
	.globl	SpikeOperBXOr
	mov	$__spk_sym___bxor__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperBXOrSuper:
	.globl	SpikeOperBXOrSuper
	mov	$__spk_sym___bxor__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x | y" ***/

SpikeOperBOr:
	.globl	SpikeOperBOr
	mov	$__spk_sym___bor__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeOperBOrSuper:
	.globl	SpikeOperBOrSuper
	mov	$__spk_sym___bor__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "f(x)" ***/

SpikeCall:
	.globl	SpikeCall
	mov	$__spk_sym___apply__, %rdx	# selector
	jmp	SpikeSendMessage

SpikeCallSuper:
	.globl	SpikeCallSuper
	mov	$__spk_sym___apply__, %rdx	# selector
	jmp	SpikeSendMessageSuper


/*** "a[i]" ***/

SpikeGetIndex:
	.globl	SpikeGetIndex
	mov	$__spk_sym___index__, %rdx	# selector
	jmp	SpikeSendMessage

SpikeGetIndexSuper:
	.globl	SpikeGetIndexSuper
	mov	$__spk_sym___index__, %rdx	# selector
	jmp	SpikeSendMessageSuper


/*** "a[i] = v" ***/

SpikeSetIndex:
	.globl	SpikeSetIndex
	mov	$__spk_sym___index__, %rdx	# selector
	jmp	SpikeSendMessageLValue

SpikeSetIndexSuper:
	.globl	SpikeSetIndexSuper
	mov	$__spk_sym___index__, %rdx	# selector
	jmp	SpikeSendMessageSuperLValue


/*** "*p" ***/

SpikeGetInd:
	.globl	SpikeGetInd
	mov	$__spk_sym___ind__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeGetIndSuper:
	.globl	SpikeGetIndSuper
	mov	$__spk_sym___ind__, %rdx	# selector
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "*p = v" ***/

SpikeSetInd:
	.globl	SpikeSetInd
	mov	$__spk_sym___ind__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageLValue

SpikeSetIndSuper:
	.globl	SpikeSetIndSuper
	mov	$__spk_sym___ind__, %rdx	# selector
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuperLValue


/*** "obj.attr", "obj.*attr" ***/

SpikeGetAttr:
	.globl	SpikeGetAttr
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessage

SpikeGetAttrSuper:
	.globl	SpikeGetAttrSuper
	mov	$0, %rcx		# argument count
	jmp	SpikeSendMessageSuper


/*** "obj.attr = v", "obj.*attr = v" ***/

SpikeSetAttr:
	.globl	SpikeSetAttr
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageLValue

SpikeSetAttrSuper:
	.globl	SpikeSetAttrSuper
	mov	$1, %rcx		# argument count
	jmp	SpikeSendMessageSuperLValue


/*
 * send message bottleneck
 *
 * On entry:
 *
 *     %rbp: activeContext / sender (MethodContext or BlockContext)
 *
 *     %rdx: selector
 *     %rcx: argumentCount
 *
 *     |------------|
 *     | receiver   | receiver / space for result
 *     |------------|
 *     | arg 1      | args pushed by caller left-to-right
 *     | arg 2      |
 *     | ...        |
 *     | arg N      |
 *     |------------|
 *     | ret addr   |
 *     |------------| <- %rsp
 */

saveRegisters:
	pop	%rax
	pop	32(%rbp)	# pop sender's return address into Context.pc
	mov	%rbx, 48(%rbp)	# save callee-preserved registers
	mov	%rsi, 56(%rbp)
	mov	%rdi, 64(%rbp)
	lea	(%rsp,%rcx,8), %rsi	# Context.sp = %rsp upon return
	mov	%rsi, 40(%rbp)
	jmp	*%rax

SpikeSendMessageSuperLValue:
	.globl	SpikeSendMessageSuperLValue
	call	saveRegisters
	mov	16(%rbp), %rbx	# get homeContext
	mov	96(%rbx), %rsi	# restore self
	mov	88(%rbx), %rbx 	# get methodClass
	mov	8(%rbx), %rbx 	# get superclass of methodClass
	mov	$1, %rdi	# lvalue namespace
	jmp	lookupMethod

SpikeSendMessageLValue:
	.globl	SpikeSendMessageLValue
	call	saveRegisters
	mov	(%rsp,%rcx,8), %rsi	# get receiver
	call	SpikeGetClass 	# get class in %rbx
	mov	$1, %rdi	# lvalue namespace
	jmp	lookupMethod

SpikeSendMessageSuper:
	.globl	SpikeSendMessageSuper
	call	saveRegisters
	mov	16(%rbp), %rbx	# get homeContext
	mov	96(%rbx), %rsi	# restore self
	mov	88(%rbx), %rbx 	# get methodClass
	mov	8(%rbx), %rbx 	# get superclass of methodClass
	mov	$0, %rdi	# rvalue namespace
	jmp	lookupMethod

SpikeSendMessage:
	.globl	SpikeSendMessage
	call	saveRegisters
	mov	(%rsp,%rcx,8), %rsi	# get receiver
	call	SpikeGetClass 	# get class in %rbx
	mov	$0, %rdi	# rvalue namespace
	/* fall through */

lookupMethod:
	push	%rcx		# save registers
	push	%rdx		# 0x10
	push	%rsi
	push	%rdi		# 0x00
.L1:
	mov	0x10(%rsp), %rdx	# selector
	mov	0x00(%rsp), %rsi	# namespace
	mov	%rbx, %rdi	# behavior
	call	SpikeLookupMethod
	test	%rax, %rax
	jne	callNewMethod
	mov	8(%rbx), %rbx 	# up superclass chain
	test	%rbx, %rbx
	jne	.L1

/* lookup failed */
	pop	%rdi
	pop	%rsi
	pop	%rdx
	pop	%rcx
	cmp	$__spk_sym_doesNotUnderstand$, %rdx
	jne	createActualMessage
	push	$__spk_sym_doesNotUnderstand$
	call	SpikeError

createActualMessage:
	push	%rcx		# save argument count
	push	%rsi		# save regs
	push	%rdi
/* create Message object */
	lea	0x18(%rsp), %rax	# arg pointer
	push	%rax
	push	%rcx		# argumentCount
	push	%rdx		# selector
	push	%rdi		# namespace
	pop	%rdi		# getting sloppy
	pop	%rsi
	pop	%rdx
	pop	%rcx
	call	SpikeCreateActualMessage
	pop	%rdi		# restore regs
	pop	%rsi

/* discard arguments pushed by caller */
	pop	%rcx
	lea	(%rsp,%rcx,8), %rsp

/* push Message object pointer as the lone argument */
	push	%rax

/* lookup doesNotUnderstand: */
	mov	$0, %rdi	# rvalue namespace
	mov	$__spk_sym_doesNotUnderstand$, %rdx  # new selector
	mov	$1, %rcx	# new argument count
	call	SpikeGetClass 	# start over
	jmp	lookupMethod

/* found it */
callNewMethod:

	pop	%rdi
	pop	%rsi

/* set up Method's instance variable pointer in %rdi */
	lea	8(%rax), %rdi

	pop	%rax		# discard selector
	pop	%rdx		# get argument count

	jmp	SpikePrologue

