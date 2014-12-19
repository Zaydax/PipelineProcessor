	.ORIG x3000
	LEA     R0, B
	LDB	R1, R0, #-2
	LDB     R2, R0, #-1
	LDB     R3, R0, #0
	LDB     R4, R0, #1
	LDB     R5, R0, #2
	LDB     R6, R0, #3
	HALT

A       .FILL   xDEFA
B       .FILL   x6E4C
C       .FILL   XCAB0			
	.END