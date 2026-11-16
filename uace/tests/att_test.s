.file	"test.s"
.section	.text
.globl	_start

_start:
	movl	$1, %eax
	movl	$1, %edi
	movl	$message, %esi
	movl	$13, %edx
	int	$0x80

.section	.data
message:
.asciz	"Hello, World!"