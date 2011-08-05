
	.text

SpikeError:
	.globl	SpikeError
	.type	SpikeError, @function

	int3
	ret

	.size	SpikeError, .-SpikeError
