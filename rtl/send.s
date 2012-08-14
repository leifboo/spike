
	.text

/*** "x++", "++x" ***/

SpikeOperSucc:
	.globl	SpikeOperSucc
	movl	$__sym___succ__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperSuccSuper:
	.globl	SpikeOperSuccSuper
	movl	$__sym___succ__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x--", "--x" ***/

SpikeOperPred:
	.globl	SpikeOperPred
	movl	$__sym___pred__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperPredSuper:
	.globl	SpikeOperPredSuper
	movl	$__sym___pred__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "&x" ***/

SpikeOperAddr:
	.globl	SpikeOperAddr
	movl	$__sym___addr__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperAddrSuper:
	.globl	SpikeOperAddrSuper
	movl	$__sym___addr__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "+x" ***/

SpikeOperPos:
	.globl	SpikeOperPos
	movl	$__sym___pos__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperPosSuper:
	.globl	SpikeOperPosSuper
	movl	$__sym___pos__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "-x" ***/

SpikeOperNeg:
	.globl	SpikeOperNeg
	movl	$__sym___neg__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperNegSuper:
	.globl	SpikeOperNegSuper
	movl	$__sym___neg__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "~x" ***/

SpikeOperBNeg:
	.globl	SpikeOperBNeg
	movl	$__sym___bneg__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperBNegSuper:
	.globl	SpikeOperBNegSuper
	movl	$__sym___bneg__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "!x" ***/

SpikeOperLNeg:
	.globl	SpikeOperLNeg
	movl	$__sym___lneg__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperLNegSuper:
	.globl	SpikeOperLNegSuper
	movl	$__sym___lneg__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x * y" ***/

SpikeOperMul:
	.globl	SpikeOperMul
	movl	$__sym___mul__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperMulSuper:
	.globl	SpikeOperMulSuper
	movl	$__sym___mul__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x / y" ***/

SpikeOperDiv:
	.globl	SpikeOperDiv
	movl	$__sym___div__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperDivSuper:
	.globl	SpikeOperDivSuper
	movl	$__sym___div__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x % y" ***/

SpikeOperMod:
	.globl	SpikeOperMod
	movl	$__sym___mod__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperModSuper:
	.globl	SpikeOperModSuper
	movl	$__sym___mod__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x + y" ***/

SpikeOperAdd:
	.globl	SpikeOperAdd
	movl	$__sym___add__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperAddSuper:
	.globl	SpikeOperAddSuper
	movl	$__sym___add__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x - y" ***/

SpikeOperSub:
	.globl	SpikeOperSub
	movl	$__sym___sub__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperSubSuper:
	.globl	SpikeOperSubSuper
	movl	$__sym___sub__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x << y" ***/

SpikeOperLShift:
	.globl	SpikeOperLShift
	movl	$__sym___lshift__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperLShiftSuper:
	.globl	SpikeOperLShiftSuper
	movl	$__sym___lshift__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x >> y" ***/

SpikeOperRShift:
	.globl	SpikeOperRShift
	movl	$__sym___rshift__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperRShiftSuper:
	.globl	SpikeOperRShiftSuper
	movl	$__sym___rshift__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x < y" ***/

SpikeOperLT:
	.globl	SpikeOperLT
	movl	$__sym___lt__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperLTSuper:
	.globl	SpikeOperLTSuper
	movl	$__sym___lt__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x > y" ***/

SpikeOperGT:
	.globl	SpikeOperGT
	movl	$__sym___gt__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperGTSuper:
	.globl	SpikeOperGTSuper
	movl	$__sym___gt__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x <= y" ***/

SpikeOperLE:
	.globl	SpikeOperLE
	movl	$__sym___le__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperLESuper:
	.globl	SpikeOperLESuper
	movl	$__sym___le__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x >= y" ***/

SpikeOperGE:
	.globl	SpikeOperGE
	movl	$__sym___ge__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperGESuper:
	.globl	SpikeOperGESuper
	movl	$__sym___ge__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x == y" ***/

SpikeOperEq:
	.globl	SpikeOperEq
	movl	$__sym___eq__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperEqSuper:
	.globl	SpikeOperEqSuper
	movl	$__sym___eq__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x != y" ***/

SpikeOperNE:
	.globl	SpikeOperNE
	movl	$__sym___ne__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperNESuper:
	.globl	SpikeOperNESuper
	movl	$__sym___ne__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x & y" ***/

SpikeOperBAnd:
	.globl	SpikeOperBAnd
	movl	$__sym___band__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperBAndSuper:
	.globl	SpikeOperBAndSuper
	movl	$__sym___band__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x ^ y" ***/

SpikeOperBXOr:
	.globl	SpikeOperBXOr
	movl	$__sym___bxor__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperBXOrSuper:
	.globl	SpikeOperBXOrSuper
	movl	$__sym___bxor__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "x | y" ***/

SpikeOperBOr:
	.globl	SpikeOperBOr
	movl	$__sym___bor__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeOperBOrSuper:
	.globl	SpikeOperBOrSuper
	movl	$__sym___bor__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "f(x)" ***/

SpikeCall:
	.globl	SpikeCall
	movl	$__sym___apply__, %edx	# selector
	jmp	SpikeSendMessage

SpikeCallSuper:
	.globl	SpikeCallSuper
	movl	$__sym___apply__, %edx	# selector
	jmp	SpikeSendMessageSuper


/*** "a[i]" ***/

SpikeGetIndex:
	.globl	SpikeGetIndex
	movl	$__sym___index__, %edx	# selector
	jmp	SpikeSendMessage

SpikeGetIndexSuper:
	.globl	SpikeGetIndexSuper
	movl	$__sym___index__, %edx	# selector
	jmp	SpikeSendMessageSuper


/*** "a[i] = v" ***/

SpikeSetIndex:
	.globl	SpikeSetIndex
	movl	$__sym___index__, %edx	# selector
	jmp	SpikeSendMessageLValue

SpikeSetIndexSuper:
	.globl	SpikeSetIndexSuper
	movl	$__sym___index__, %edx	# selector
	jmp	SpikeSendMessageSuperLValue


/*** "*p" ***/

SpikeGetInd:
	.globl	SpikeGetInd
	movl	$__sym___ind__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeGetIndSuper:
	.globl	SpikeGetIndSuper
	movl	$__sym___ind__, %edx	# selector
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "*p = v" ***/

SpikeSetInd:
	.globl	SpikeSetInd
	movl	$__sym___ind__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageLValue

SpikeSetIndSuper:
	.globl	SpikeSetIndSuper
	movl	$__sym___ind__, %edx	# selector
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuperLValue


/*** "obj.attr", "obj.*attr" ***/

SpikeGetAttr:
	.globl	SpikeGetAttr
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessage

SpikeGetAttrSuper:
	.globl	SpikeGetAttrSuper
	movl	$0, %ecx		# argument count
	jmp	SpikeSendMessageSuper


/*** "obj.attr = v", "obj.*attr = v" ***/

SpikeSetAttr:
	.globl	SpikeSetAttr
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageLValue

SpikeSetAttrSuper:
	.globl	SpikeSetAttrSuper
	movl	$1, %ecx		# argument count
	jmp	SpikeSendMessageSuperLValue


/*
 * send message bottleneck
 *
 * On entry:
 *
 *     %ebp: activeContext / sender (MethodContext or BlockContext)
 *
 *     %edx: selector
 *     %ecx: argumentCount
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
 *     |------------| <- %esp
 */

saveRegisters:
	popl	%eax
	popl	16(%ebp)	# pop sender's return address into Context.pc
	movl	%ebx, 24(%ebp)	# save callee-preserved registers
	movl	%esi, 28(%ebp)
	movl	%edi, 32(%ebp)
	leal	(%esp,%ecx,4), %esi	# Context.sp = %esp upon return
	movl	%esi, 20(%ebp)
	jmp	*%eax

SpikeSendMessageSuperLValue:
	.globl	SpikeSendMessageSuperLValue
	call	saveRegisters
	movl	8(%ebp), %ebx	# get homeContext
	movl	48(%ebx), %esi	# restore self
	movl	44(%ebx), %ebx 	# get methodClass
	movl	4(%ebx), %ebx 	# get superclass of methodClass
	movl	$1, %edi	# lvalue namespace
	jmp	lookupMethod

SpikeSendMessageLValue:
	.globl	SpikeSendMessageLValue
	call	saveRegisters
	movl	(%esp,%ecx,4), %esi	# get receiver
	call	SpikeGetClass 	# get class in %ebx
	movl	$1, %edi	# lvalue namespace
	jmp	lookupMethod

SpikeSendMessageSuper:
	.globl	SpikeSendMessageSuper
	call	saveRegisters
	movl	8(%ebp), %ebx	# get homeContext
	movl	48(%ebx), %esi	# restore self
	movl	44(%ebx), %ebx 	# get methodClass
	movl	4(%ebx), %ebx 	# get superclass of methodClass
	movl	$0, %edi	# rvalue namespace
	jmp	lookupMethod

SpikeSendMessage:
	.globl	SpikeSendMessage
	call	saveRegisters
	movl	(%esp,%ecx,4), %esi	# get receiver
	call	SpikeGetClass 	# get class in %ebx
	movl	$0, %edi	# rvalue namespace
	/* fall through */

lookupMethod:
	pushl	%ecx		# save registers
	pushl	%edx
.L1:
	pushl	(%esp)		# selector
	pushl	%edi		# namespace
	pushl	%ebx		# behavior
	call	SpikeLookupMethod
	addl	$12, %esp
	testl	%eax, %eax
	jne	callNewMethod
	movl	4(%ebx), %ebx 	# up superclass chain
	testl	%ebx, %ebx
	jne	.L1

/* lookup failed */
	popl	%edx
	popl	%ecx
	cmpl	$__sym_doesNotUnderstand$, %edx
	jne	createActualMessage
	pushl	$__sym_doesNotUnderstand$
	call	SpikeError

createActualMessage:
	pushl	%ecx		# save argument count
/* create Message object */
	leal	4(%esp), %eax	# arg pointer
	pushl	%eax
	pushl	%ecx		# argumentCount
	pushl	%edx		# selector
	pushl	%edi		# namespace
	call	SpikeCreateActualMessage
	addl	$16, %esp

/* discard arguments pushed by caller */
	popl	%ecx
	leal	(%esp,%ecx,4), %esp

/* push Message object pointer as the lone argument */
	pushl	%eax

/* lookup doesNotUnderstand: */
	movl	$0, %edi	# rvalue namespace
	movl	$__sym_doesNotUnderstand$, %edx  # new selector
	movl	$1, %ecx	# new argument count
	call	SpikeGetClass 	# start over
	jmp	lookupMethod

/* found it */
callNewMethod:

/* set up Method's instance variable pointer in %edi */
	leal	4(%eax), %edi

	popl	%eax		# discard selector
	popl	%edx		# get argument count

	jmp	SpikePrologue

