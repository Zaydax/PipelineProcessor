	.ORIG   x3000
	LEA     R3, TARGET
	LDW	R3, R3, #0
	AND 	R0, R0, #0
	LEA 	R1, VALUE
	LDW	R2, R1, #0
	BRnz	ERROR							
	ADD	R0, R0, #15
LOOP	ADD	R0, R0, #-1
	LSHF	R2, R2, #1	
	BRp	LOOP
	BRnzp	DONE
ERROR   ADD	R0, R0, #-1	
DONE	STB	R0, R3, #0	
	HALT
VALUE 	.FILL x0344
TARGET	.FILL x1234
	.END
