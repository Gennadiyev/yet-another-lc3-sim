        .ORIG   x3000
        LEA R0, STRZ
        PUTS
        IN
        LEA R0, STRZ2
        PUTS
        HALT
STRZ    .STRINGZ    "\nMonster! Don't touch your keyboard!\n"
STRZ2   .STRINGZ    "\nNOOOOOOOOOO I told you not to!\n"
        .END
