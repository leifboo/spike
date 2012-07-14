
	.text

SpikeEpilogue:
	.globl	SpikeEpilogue
	.type	SpikeEpilogue, @function

/*
 * Return to the caller, destroying the active MethodContext.
 * Preserve %eax, %ecx, %edx.  Restore all other registers.
 */

/* restore registers */
	movl	4(%ebp), %ebp	# activeContext = caller
	movl	24(%ebp), %ebx	# restore registers
	movl	28(%ebp), %esi
	movl	32(%ebp), %edi

/* restore stack pointer, popping context */
	movl	20(%ebp), %esp

/* return */
	pushl	16(%ebp)	# pc
	ret

	.size	SpikeEpilogue, .-SpikeEpilogue
