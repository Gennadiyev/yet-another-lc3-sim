/*
 * LC-3 Simulator
 * 
 * Author: Ji Yikun (da-kun#sjtu.edu.cn)
 *
 * As a lab project of CS169, Computer Architecture by Jingwen Leng.
 *
 * Summary:     The project can load an ISA program in pure decimal text format
 *              as mentioned in the original document.
 *              See original document for usage.
 * Known issue: PSR not implemented.
 * License:     All the codes are open-sourced at https://github.com/Gennadiyev/yet-another-lc3-sim.git
 *              under the MIT License.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

/* Simulate inputs without echo */
struct termios orig_termios;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

int getch()
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}
/* End simulates non-echoing inputs */

/***************************************************************/
/*                                                             */
/* Files: isaprogram   LC-3 machine language program file     */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void processInstruction();

/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define FALSE 0
#define TRUE  1

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the bus.           */
/***************************************************************/
#define Low16bits(x) ((x) & 0xFFFF)

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/*
  MEMORY[A] stores the word address A
*/

#define WORDS_IN_MEM    0x08000
int MEMORY[WORDS_IN_MEM];

/***************************************************************/

/***************************************************************/

/***************************************************************/
/* LC-3 State info.                                           */
/***************************************************************/
#define LC_3_REGS 8

int RUN_BIT;	/* run bit */

typedef struct System_Latches_Struct {
    int PC,		/* program counter */
    N,		/* n condition bit */
    Z,		/* z condition bit */
    P,      /* p condition bit */
    IR,		/* instruction register */
    PSR;    /* Priviledged register */
    int REGS[LC_3_REGS]; /* register file. */
} System_Latches;

/* Data Structure for Latch */

System_Latches CURRENT_LATCHES, NEXT_LATCHES;

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int INSTRUCTION_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands                    */
/*                                                             */
/***************************************************************/
void help() {
    printf("----------------LC-3 ISIM Help-----------------------\n");
    printf("go               -  run program to completion         \n");
    printf("run n            -  execute program for n instructions\n");
    printf("mdump low high   -  dump memory from low to high      \n");
    printf("rdump            -  dump the register & bus values    \n");
    printf("?                -  display this help menu            \n");
    printf("quit             -  exit the program                  \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {

    processInstruction();
    CURRENT_LATCHES = NEXT_LATCHES;
    INSTRUCTION_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3 for n cycles                 */
/*                                                             */
/***************************************************************/
void run(int num_cycles) {
    int i;

    if (RUN_BIT == FALSE) {
        printf("Can't simulate, Simulator is halted\n\n");
        return;
    }

    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++) {
        if (CURRENT_LATCHES.PC == 0x0000) {
            RUN_BIT = FALSE;
            printf("\nSimulator halted\n\n");
            break;
        }
        cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the LC-3 until HALTed                 */
/*                                                             */
/***************************************************************/
void go() {
    if (RUN_BIT == FALSE) {
        printf("Can't simulate, Simulator is halted\n\n");
        return;
    }

    printf("Simulating...\n");
    while (CURRENT_LATCHES.PC != 0x0000)
        cycle();
    RUN_BIT = FALSE;
    printf("\nSimulator halted\n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a word-aligned region of memory to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void mdump(FILE * dumpsim_file, int start, int stop) {
    int address; /* this is a address */

    printf("\nMemory content [0x%.4x..0x%.4x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = start ; address <= stop ; address++)
        printf("  0x%.4x (%d) : 0x%.2x\n", address , address , MEMORY[address]);
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%.4x..0x%.4x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = start ; address <= stop ; address++)
        fprintf(dumpsim_file, " 0x%.4x (%d) : 0x%.2x\n", address , address , MEMORY[address]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current register and bus values to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE * dumpsim_file) {
    int k;

    printf("\nCurrent register/bus values :\n");
    printf("-------------------------------------\n");
    printf("Instruction Count : %d\n", INSTRUCTION_COUNT);
    printf("PC                : 0x%.4x\n", CURRENT_LATCHES.PC);
    printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    printf("Registers:\n");
    for (k = 0; k < LC_3_REGS; k++)
        printf("%d: 0x%.4x\n", k, CURRENT_LATCHES.REGS[k]);
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Instruction Count : %d\n", INSTRUCTION_COUNT);
    fprintf(dumpsim_file, "PC                : 0x%.4x\n", CURRENT_LATCHES.PC);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC_3_REGS; k++)
        fprintf(dumpsim_file, "%d: 0x%.4x\n", k, CURRENT_LATCHES.REGS[k]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

/***************************************************************/
/*                                                             */
/* Procedure : getCommand                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */
/*                                                             */
/***************************************************************/
void getCommand(FILE * dumpsim_file) {
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3-SIM> ");

    scanf("%s", buffer);
    printf("\n");

    switch(buffer[0]) {
        case 'G':
        case 'g':
            go();
            break;

        case 'M':
        case 'm':
            scanf("%i %i", &start, &stop);
            mdump(dumpsim_file, start, stop);
            break;

        case '?':
            help();
            break;
        case 'Q':
        case 'q':
            printf("Bye.\n");
            exit(0);

        case 'R':
        case 'r':
            if (buffer[1] == 'd' || buffer[1] == 'D')
                rdump(dumpsim_file);
            else {
                scanf("%d", &cycles);
                run(cycles);
            }
            break;

        default:
            printf("Invalid Command\n");
            break;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : initMemory                                     */
/*                                                             */
/* Purpose   : Zero out the memory array                       */
/*                                                             */
/***************************************************************/
void initMemory() {
    int i;

    for (i=0; i < WORDS_IN_MEM; i++) {
        MEMORY[i] = 0;
    }
}

/**************************************************************/
/*                                                            */
/* Procedure : loadProgram                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void loadProgram(char *program_filename) {
    FILE * prog;
    int ii, word, program_base;

    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL) {
        printf("Error: Can't open program file %s\n", program_filename);
        exit(-1);
    }

    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF)
        program_base = word ;
    else {
        printf("Error: Program file is empty\n");
        exit(-1);
    }

    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF) {
        /* Make sure it fits. */
        if (program_base + ii >= WORDS_IN_MEM) {
            printf("Error: Program file %s is too long to fit in memory. %x\n",
                   program_filename, ii);
            exit(-1);
        }

        /* Write the word to memory array. */
        MEMORY[program_base + ii] = word;
        ii++;
    }

    if (CURRENT_LATCHES.PC == 0) CURRENT_LATCHES.PC = program_base;

    printf("Read %d words from program into memory.\n\n", ii);
}

/************************************************************/
/*                                                          */
/* Procedure : initialize                                   */
/*                                                          */
/* Purpose   : Load machine language program                */
/*             and set up initial state of the machine.     */
/*                                                          */
/************************************************************/
void initialize(char *program_filename, int num_prog_files) {
    int i;

    initMemory();
    for ( i = 0; i < num_prog_files; i++ ) {
        loadProgram(program_filename);
        while(*program_filename++ != '\0');
    }
    CURRENT_LATCHES.Z = 1;
    NEXT_LATCHES = CURRENT_LATCHES;

    RUN_BIT = TRUE;
}

/***************************************************************/
/*                                                             */
/* Procedure : main                                            */
/*                                                             */
/***************************************************************/
int main(int argc, char *argv[]) {
    FILE * dumpsim_file;

    /* Error Checking */
    if (argc < 2) {
        printf("Error: usage: %s <program_file_1> <program_file_2> ...\n",
               argv[0]);
        exit(1);
    }

    printf("LC-3 Simulator\n\n");

    initialize(argv[1], argc - 1);

    if ( (dumpsim_file = fopen( "dumpsim", "w" )) == NULL ) {
        printf("Error: Can't open dumpsim file\n");
        exit(-1);
    }

    while (1)
        getCommand(dumpsim_file);

}

int Instruction;

/* System stack: 0x2F00 - 0x2FFF */ 

int top_p = 0x2FFF;

int isEmpty() {
    return top_p == 0x2FFF;
}

int POP () {
    top_p += 1;
    if (top_p > 0x2FFF) {
        printf("Error: system stack segmentation fault");
        return -1;
    } 
    return MEMORY[top_p - 1];
}

int PUSH(int value) {
    top_p -= 1;
    if (top_p <= 0x2F00) {
        printf("Error: system stack overflow\n");
        return -1;
    }
    MEMORY[top_p] = value;
    return 0;
}

int SetCC (int value);  /* Update condition code */
int SEXT (int num, int length);  /* Sign extension */

int ADD (int instruction);  /* 0001 */
int AND (int instruction);  /* 0101 */
int BR (int instruction);   /* 0000 */
int JMP (int instruction);  /* 1100 */
int JSR (int instruction);  /* 0100 */
int JSRR (int instruction); /* 0100 */
int LD (int instruction);   /* 0010 */
int LDI (int instruction);  /* 1010 */
int LDR (int instruction);  /* 0110 */
int LEA (int instruction);  /* 1110 */
int NOT (int instruction);  /* 1001 */
// RET has same opcode with JMP
int RTI (int instruction);  /* 1000 */ /* FLAWED */
int ST (int instruction);   /* 0011 */
int STI (int instruction);  /* 1011 */
int STR (int instruction);  /* 0111 */
int TRAP (int instruction); /* 1111 */

void printASCII (int asc) {
    // reset_terminal_mode();
    if (asc == 13) {
        printf("\r\n");
    } else {
        printf("%c", asc, asc);
    }
    fflush(stdout);
}

int getMemory (int address) {
    if (address == 0xFE04) { // DSR
        return 0x0000;
    } else if (address >= 0xFD00 || address < 0x3000) {
        printf("\nWarning: attempt to read address %x\n", address);
        return MEMORY[address];
    } else {
        return MEMORY[address];
    }
}

void setMemory (int address, int value) {
    if (address == 0xFE06) { // DDR
        printASCII(value);
    } else if (address >= 0xFD00 || address < 0x3000) {
        printf("\nWarning: attempt to write to address %x\n", address);
        MEMORY[address] = value;
    } else {
        MEMORY[address] = value;
    }
}

void processInstruction() {

    Instruction = getMemory(CURRENT_LATCHES.PC);
    CURRENT_LATCHES.PC += 1;
    Instruction = Low16bits(Instruction);
    
    int operation = Instruction & 0xF000;  /* Instruction[15:12] */
    operation = operation >> 12;

    /* Execute */
    switch (operation) {
        case 0b0001:    /* 0001 */
            ADD(Instruction);
            break;

        case 0b0101:    /* 0101 */
            AND(Instruction);
            break;

        case 0b0000:    /* 0000 */
            BR(Instruction);
            break;

        case 0b1100:    /* 1100 */
            JMP(Instruction);
            break;

        case 0b0100:    /* 0100 */
            if ((Instruction & 0x0800) >> 11)
                JSR(Instruction);
            else
                JSRR(Instruction);
            break;

        case 0b0010:    /* 0010 */
            LD(Instruction);
            break;

        case 0b1010:    /* 1010 */
            LDI(Instruction);
            break;

        case 0b0110:    /* 0110 */
            LDR(Instruction);
            break;

        case 0b1110:    /* 1110 */
            LEA(Instruction);
            break;

        case 0b1001:    /* 1001 */
            NOT(Instruction);
            break;

        case 0b1000:    /* 1000 */
            RTI(Instruction);
            break;

        case 0b0011:    /* 0011 */
            ST(Instruction);
            break;

        case 0b1011:    /* 1011 */
            STI(Instruction);
            break;

        case 0b0111:    /* 0111 */
            STR(Instruction);
            break;

        case 0b1111:    /* 1111 */
            TRAP(Instruction);
            break;

        default:
            break;
    }

    NEXT_LATCHES = CURRENT_LATCHES;
}

int SetCC (int value) {

    if (!value) {
        CURRENT_LATCHES.N = 0;
        CURRENT_LATCHES.Z = 1;
        CURRENT_LATCHES.P = 0;
        return 0;
    }

    value = Low16bits(value) & 0x8000;  /* Sign bit */

    if (!value) {  /* Positive */
        CURRENT_LATCHES.N = 0;
        CURRENT_LATCHES.Z = 0;
        CURRENT_LATCHES.P = 1;
        return 0;
    } else {   /* Negative */
        CURRENT_LATCHES.N = 1;
        CURRENT_LATCHES.Z = 0;
        CURRENT_LATCHES.P = 0;
        return 0;
    }
}

int SEXT (int num, int length) {  /* Sign extension */
    int Sign;
    switch (length) {
        case 5:     /* imm5 */
            Sign = (num & 0x0010) >> 4;
            if(Sign)        /* Negative */
                num += 0xFFE0;  /* Extend with 1 */
            break;

        case 6:     /* PCoffset6 */
            Sign = (num & 0x0020) >> 5;
            if(Sign)
                num += 0xFFC0;
            break;

        case 9:     /* PCoffset9 */
            Sign = (num & 0x0100) >> 8;
            if(Sign)
                num += 0xFE00;
            break;

        case 11:    /* PCoffset11 */
            Sign = (num & 0x0400) >> 10;
            if(Sign)
                num += 0xF800;
            break;

        default:
            break;
    }
    num = Low16bits(num);
    return num;
}

int ADD (int instruction) {
    int DR = (instruction & 0x0E00) >> 9;
    int SR1 = (instruction & 0x01C0) >> 6;

    if((instruction & 0x0020) == 0) {      /* Instruction[5] = 0 */
        int SR2 = (instruction & 0x0007);
        CURRENT_LATCHES.REGS[DR]
                = CURRENT_LATCHES.REGS[SR1] + CURRENT_LATCHES.REGS[SR2];
    }

    else{      /* Instruction[5] = 1 */
        int imm5 = (instruction & 0x001F);
        CURRENT_LATCHES.REGS[DR] = CURRENT_LATCHES.REGS[SR1] + SEXT(imm5, 5);
    }

    CURRENT_LATCHES.REGS[DR] = Low16bits(CURRENT_LATCHES.REGS[DR]);
    SetCC(CURRENT_LATCHES.REGS[DR]);
    return 0;
}

int AND (int instruction) {
    int DR = (instruction & 0x0E00) >> 9;
    int SR1 = (instruction & 0x01C0) >> 6;

    if((instruction & 0x0020) == 0) {            /* Instruction[5] = 0 */
        int SR2 = (instruction & 0x0007);
        CURRENT_LATCHES.REGS[DR] = CURRENT_LATCHES.REGS[SR1] & CURRENT_LATCHES.REGS[SR2];
    }

    else{       /* Instruction[5] = 1 */
        int imm5 = (instruction & 0x001F);
        CURRENT_LATCHES.REGS[DR] = CURRENT_LATCHES.REGS[SR1] & SEXT(imm5, 5);
    }

    CURRENT_LATCHES.REGS[DR] = Low16bits(CURRENT_LATCHES.REGS[DR]);
    SetCC(CURRENT_LATCHES.REGS[DR]);
    return 0;
}

int BR (int instruction) {
    int PCoffset9 = instruction & 0x01FF;
    int n = (instruction & 0x0800) >> 11;
    int z = (instruction & 0x0400) >> 10;
    int p = (instruction & 0x0200) >> 9;
    int flag = n && CURRENT_LATCHES.N || z && CURRENT_LATCHES.Z || p && CURRENT_LATCHES.P;  /* Condition */

    if (flag) {
        CURRENT_LATCHES.PC += SEXT(PCoffset9, 9);
        CURRENT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC);
    }

    return 0;
}

int JMP (int instruction) {
    int BaseR = (instruction & 0x01C0) >> 6;
    if (BaseR == 7) {        // RET
        if (isEmpty()) {
            printf("Error: RET called when stack is empty");
        } else {
            CURRENT_LATCHES.REGS[7] = POP();
        }
    }

    CURRENT_LATCHES.PC = Low16bits(CURRENT_LATCHES.REGS[BaseR]);
    return 0;
}

int JSR (int instruction) {
    CURRENT_LATCHES.REGS[7] = CURRENT_LATCHES.PC;  /* Save R7 first */
    PUSH(CURRENT_LATCHES.REGS[7]);

    int PCoffset11 = instruction & 0x07FF;
    CURRENT_LATCHES.PC += SEXT(PCoffset11, 11);
    CURRENT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC);
    return 0;
}

int JSRR (int instruction) {
    CURRENT_LATCHES.REGS[7] = CURRENT_LATCHES.PC;   /* Save R7 first */
    PUSH(CURRENT_LATCHES.REGS[7]);

    int BaseR = (instruction & 0x01C0) >> 6;
    CURRENT_LATCHES.PC = Low16bits(CURRENT_LATCHES.REGS[BaseR]);
    return 0;
}

int LD (int instruction) {
    int DR = (instruction & 0x0E00) >> 9;
    int PCoffset9 = instruction & 0x01FF;
    int address = Low16bits((CURRENT_LATCHES.PC + SEXT(PCoffset9, 9)));
    CURRENT_LATCHES.REGS[DR] = Low16bits(getMemory(address));

    SetCC(CURRENT_LATCHES.REGS[DR]);
    return 0;
}

int LDI (int instruction) {
    int DR = (instruction & 0x0E00) >> 9;
    int PCoffset9 = instruction & 0x01FF;

    int address = Low16bits((CURRENT_LATCHES.PC + SEXT(PCoffset9, 9)));
    CURRENT_LATCHES.REGS[DR] = Low16bits(getMemory(getMemory(address)));

    SetCC(CURRENT_LATCHES.REGS[DR]);
    return 0;
}

int LDR (int instruction) {
    int DR = (instruction & 0x0E00) >> 9;
    int BaseR = (instruction & 0x01C0) >> 6;
    int PCoffset6 = instruction & 0x003F;

    int address = Low16bits((CURRENT_LATCHES.REGS[BaseR] + SEXT(PCoffset6, 6)));
    CURRENT_LATCHES.REGS[DR] = Low16bits(getMemory(address));

    SetCC(CURRENT_LATCHES.REGS[DR]);
    return 0;
}

int LEA (int instruction) {
    int DR = (instruction & 0x0E00) >> 9;
    int PCoffset9 = instruction & 0x01FF;
    CURRENT_LATCHES.REGS[DR] = Low16bits(CURRENT_LATCHES.PC + SEXT(PCoffset9, 9));
    return 0;
}

int NOT (int instruction) {
    int DR = (instruction & 0x0E00) >> 9;
    int SR = (instruction & 0x01C0) >> 6;
    CURRENT_LATCHES.REGS[DR] = Low16bits(~ CURRENT_LATCHES.REGS[SR]);

    SetCC(CURRENT_LATCHES.REGS[DR]);
    return 0;
}

int RTI (int instruction) {
    if (isEmpty()) {
        printf("Error: RTI called when stack is empty");
    } else {
        CURRENT_LATCHES.PC = Low16bits(POP());
    }
    return 0;
}

int ST (int instruction) {
    int SR = (instruction & 0x0E00) >> 9;
    int PCoffset9 = instruction & 0x01FF;
    int address = Low16bits((CURRENT_LATCHES.PC + SEXT(PCoffset9, 9)));
    setMemory(address, Low16bits(CURRENT_LATCHES.REGS[SR]));
    return 0;
}

int STI (int instruction) {
    int SR = (instruction & 0x0E00) >> 9;
    int PCoffset9 = instruction & 0x01FF;
    int address = Low16bits((CURRENT_LATCHES.PC + SEXT(PCoffset9, 9)));
    setMemory(getMemory(address), Low16bits(CURRENT_LATCHES.REGS[SR]));
    return 0;
}

int STR (int instruction) {
    int SR = (instruction & 0x0E00) >> 9;
    int BaseR = (instruction & 0x01C0) >> 6;
    int PCoffset6 = instruction & 0x003F;
    int address = Low16bits((CURRENT_LATCHES.REGS[BaseR] + SEXT(PCoffset6, 6)));
    setMemory(address, Low16bits(CURRENT_LATCHES.REGS[SR]));
    return 0;
}

int TRAP(int instruction) {
    int trapVect = (instruction & 0x00FF);
    switch (trapVect)
    {
        case 0x20: { // GETC
            set_conio_terminal_mode();
            fflush(stdin);
            fflush(stdout);
            while (!kbhit()) {
                /* do some work */
            }
            int x = getch();
            if (x == 3 || x == 4) {
                printf("Error: keyboard interruption");
                exit(0);
            }
            fflush(stdin);
            fflush(stdout);
            CURRENT_LATCHES.REGS[0] = x;
            reset_terminal_mode();
            break;
        }
        case 0x21: { // OUT
            printASCII(CURRENT_LATCHES.REGS[0]);
            break;
        }
        case 0x22: { // PUTS
            int address = Low16bits(CURRENT_LATCHES.REGS[0]);
            while (getMemory(address)) {
                printASCII(getMemory(address));
                address++;
                // printf("%x\n", address);
                address = Low16bits(address);
            } 
            break;
        }
        case 0x23: { // IN
            printf("Input a character: ");
            fflush(stdout);
            set_conio_terminal_mode();
            while (!kbhit()) {
                /* do some work */
            }
            int x = getch();
            if (x == 3 || x == 4) {
                printf("Error: keyboard interruption");
                exit(0);
            }
            reset_terminal_mode();
            printf("%c", x);
            fflush(stdin);
            fflush(stdout);
            CURRENT_LATCHES.REGS[0] = x;
            break;
        }
        case 0x24: { // PUTSP
            int address = Low16bits(CURRENT_LATCHES.REGS[0]);
            while (getMemory(address)) {
                int value = getMemory(address);
                printASCII(value & 0x00FF);
                printASCII(value >> 8);
                address++;
                // printf("%x\n", address);
                address = Low16bits(address);
            } 
            break;
        }
        case 0x25: { // HALT
            CURRENT_LATCHES.PC = 0x0000;
            // RUN_BIT = FALSE; // Already handled
        }
    }
    return 0;
}

