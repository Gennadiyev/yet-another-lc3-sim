	    .ORIG	x3000
	    LEA	R0, STRIN
LOOP	ADD	R0, R0, #0
	    TRAP	x22
	    ADD	R0, R0, #1
	    LDR	R1, R0, #0
	    BRNP	LOOP
	    HALT
STRIN	.STRINGZ	"Hello Kun!"
	    .END
