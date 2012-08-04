
	.text

SpikeError:
	.globl	SpikeError
	.type	SpikeError, @function

	int3
SpikeTrap:
	.globl	SpikeTrap
	ret

	.size	SpikeError, .-SpikeError
