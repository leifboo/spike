
	.text

SpikeEpilogue:
	.globl	SpikeEpilogue
	.type	SpikeEpilogue, @function

/*
 * Return to the caller, destroying the active MethodContext.
 * Preserve %rax, %rcx, %rdx.  Restore all other registers.
 */

/* restore registers */
	mov	8(%rbp), %rbp	# activeContext = caller
	mov	48(%rbp), %rbx	# restore registers
	mov	56(%rbp), %rsi
	mov	64(%rbp), %rdi

/* restore stack pointer, popping context */
	mov	40(%rbp), %rsp

/* return */
	push	32(%rbp)	# pc
	ret

	.size	SpikeEpilogue, .-SpikeEpilogue
