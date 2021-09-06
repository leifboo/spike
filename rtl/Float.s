
	.text
	.align	8
Float.0.unboxed:
	.globl	Float.0.unboxed
	.type	Float.0.unboxed, @object
	.size	Float.0.unboxed, 32
	.quad	__spk_x_Method
	.quad	0
	.quad	0
	.quad	0
Float.0.unboxed.code:
	.globl	Float.0.unboxed.code
	.type	Float.0.unboxed.code, @function

	mov	%rsi, 128(%rbp)	# fake, safe result for Spike code
	mov	(%rdi), %rax	# real result for C/asm code

	cmp	$0, %r12
	je	.L6

	cmp	$6, %r12
	jne	.L5
	movsd	(%rdi), %xmm5
	sub	$1, %r12
	jmp	.L6
.L5:
	cmp	$5, %r12
	jne	.L4
	movsd	(%rdi), %xmm4
	sub	$1, %r12
	jmp	.L6
.L4:
	cmp	$4, %r12
	jne	.L3
	movsd	(%rdi), %xmm3
	sub	$1, %r12
	jmp	.L6
.L3:
	cmp	$3, %r12
	jne	.L2
	movsd	(%rdi), %xmm2
	sub	$1, %r12
	jmp	.L6
.L2:
	cmp	$2, %r12
	jne	.L1
	movsd	(%rdi), %xmm1
	sub	$1, %r12
	jmp	.L6

.L1:
	cmp	$1, %r12
	jne	.L6
	movsd	(%rdi), %xmm0
	sub	$1, %r12

.L6:
	mov	$16, %rcx	# result size
	ret
	.size	Float.0.unboxed.code, .-Float.0.unboxed.code

