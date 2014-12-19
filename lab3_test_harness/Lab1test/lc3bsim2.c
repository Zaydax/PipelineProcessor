/***************************************************************/
/*                                                             */
/* LC-3b Simulator (Adapted from Prof. Yale Patt at UT Austin) */
/*                                                             */
/***************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/***************************************************************/
/*                                                             */
/* Files:  ucode        Microprogram file                      */
/*         isaprogram   LC-3b machine language program file    */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void eval_micro_sequencer();
void cycle_memory();
void eval_bus_drivers();
void drive_bus();
void latch_datapath_values();

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
/* Definition of the control store layout.                     */
/***************************************************************/
#define CONTROL_STORE_ROWS 64
#define INITIAL_STATE_NUMBER 18

/***************************************************************/
/* Definition of bit order in control store word.              */
/***************************************************************/
enum CS_BITS {                                                  
    IRD,
    COND1, COND0,
    J5, J4, J3, J2, J1, J0,
    LD_MAR,
    LD_MDR,
    LD_IR,
    LD_BEN,
    LD_REG,
    LD_CC,
    LD_PC,
    GATE_PC,
    GATE_MDR,
    GATE_ALU,
    GATE_MARMUX,
    GATE_SHF,
    PCMUX1, PCMUX0,
    DRMUX,
    SR1MUX,
    ADDR1MUX,
    ADDR2MUX1, ADDR2MUX0,
    MARMUX,
    ALUK1, ALUK0,
    MIO_EN,
    R_W,
    DATA_SIZE,
    LSHF1,
    CONTROL_STORE_BITS
} CS_BITS;

/***************************************************************/
/* Functions to get at the control bits.                       */
/***************************************************************/
int GetIRD(int *x)           { return(x[IRD]); }
int GetCOND(int *x)          { return((x[COND1] << 1) + x[COND0]); }
int GetJ(int *x)             { return((x[J5] << 5) + (x[J4] << 4) +
				      (x[J3] << 3) + (x[J2] << 2) +
				      (x[J1] << 1) + x[J0]); }
int GetLD_MAR(int *x)        { return(x[LD_MAR]); }
int GetLD_MDR(int *x)        { return(x[LD_MDR]); }
int GetLD_IR(int *x)         { return(x[LD_IR]); }
int GetLD_BEN(int *x)        { return(x[LD_BEN]); }
int GetLD_REG(int *x)        { return(x[LD_REG]); }
int GetLD_CC(int *x)         { return(x[LD_CC]); }
int GetLD_PC(int *x)         { return(x[LD_PC]); }
int GetGATE_PC(int *x)       { return(x[GATE_PC]); }
int GetGATE_MDR(int *x)      { return(x[GATE_MDR]); }
int GetGATE_ALU(int *x)      { return(x[GATE_ALU]); }
int GetGATE_MARMUX(int *x)   { return(x[GATE_MARMUX]); }
int GetGATE_SHF(int *x)      { return(x[GATE_SHF]); }
int GetPCMUX(int *x)         { return((x[PCMUX1] << 1) + x[PCMUX0]); }
int GetDRMUX(int *x)         { return(x[DRMUX]); }
int GetSR1MUX(int *x)        { return(x[SR1MUX]); }
int GetADDR1MUX(int *x)      { return(x[ADDR1MUX]); }
int GetADDR2MUX(int *x)      { return((x[ADDR2MUX1] << 1) + x[ADDR2MUX0]); }
int GetMARMUX(int *x)        { return(x[MARMUX]); }
int GetALUK(int *x)          { return((x[ALUK1] << 1) + x[ALUK0]); }
int GetMIO_EN(int *x)        { return(x[MIO_EN]); }
int GetR_W(int *x)           { return(x[R_W]); }
int GetDATA_SIZE(int *x)     { return(x[DATA_SIZE]); } 
int GetLSHF1(int *x)         { return(x[LSHF1]); }

/***************************************************************/
/* The control store rom.                                      */
/***************************************************************/
int CONTROL_STORE[CONTROL_STORE_ROWS][CONTROL_STORE_BITS];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A 
   There are two write enable signals, one for each byte. WE0 is used for 
   the least significant byte of a word. WE1 is used for the most significant 
   byte of a word. */

#define WORDS_IN_MEM    0x08000 
#define MEM_CYCLES      5
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/

/***************************************************************/

/***************************************************************/
/* LC-3b State info.                                           */
/***************************************************************/
#define LC_3b_REGS 8

int RUN_BIT;	/* run bit */
int BUS;	/* value of the bus */

typedef struct System_Latches_Struct{

int PC,		/* program counter */
    MDR,	/* memory data register */
    MAR,	/* memory address register */
    IR,		/* instruction register */
    N,		/* n condition bit */
    Z,		/* z condition bit */
    P,		/* p condition bit */
    BEN;        /* ben register */

int READY;	/* ready bit */
  /* The ready bit is also latched as you dont want the memory system to assert it 
     at a bad point in the cycle*/

int REGS[LC_3b_REGS]; /* register file. */

int MICROINSTRUCTION[CONTROL_STORE_BITS]; /* The microintruction */

int STATE_NUMBER; /* Current State Number - Provided for debugging */ 
} System_Latches;

/* Data Structure for Latch */

System_Latches CURRENT_LATCHES, NEXT_LATCHES;

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int CYCLE_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands.                   */
/*                                                             */
/***************************************************************/
void help() {                                                    
    printf("----------------LC-3bSIM Help-------------------------\n");
    printf("go               -  run program to completion       \n");
    printf("run n            -  execute program for n cycles    \n");
    printf("mdump low high   -  dump memory from low to high    \n");
    printf("rdump            -  dump the register & bus values  \n");
    printf("?                -  display this help menu          \n");
    printf("quit             -  exit the program                \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {                                                

  eval_micro_sequencer();   
  cycle_memory();
  eval_bus_drivers();
  drive_bus();
  latch_datapath_values();

  CURRENT_LATCHES = NEXT_LATCHES;

  CYCLE_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles.                 */
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
	    printf("Simulator halted\n\n");
	    break;
	}
	cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the LC-3b until HALTed.                 */
/*                                                             */
/***************************************************************/
void go() {                                                     
    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating...\n\n");
    while (CURRENT_LATCHES.PC != 0x0000)
	cycle();
    RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
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
    int address; /* this is a byte address */

    printf("\nMemory content [0x%04x..0x%04x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	printf("  0x%04x (%d) : 0x%02x%02x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%04x..0x%04x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	fprintf(dumpsim_file, " 0x%04x (%d) : 0x%02x%02x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
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
    printf("Cycle Count  : %d\n", CYCLE_COUNT);
    printf("PC           : 0x%04x\n", CURRENT_LATCHES.PC);
    printf("IR           : 0x%04x\n", CURRENT_LATCHES.IR);
    printf("STATE_NUMBER : 0x%04x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    printf("BUS          : 0x%04x\n", BUS);
    printf("MDR          : 0x%04x\n", CURRENT_LATCHES.MDR);
    printf("MAR          : 0x%04x\n", CURRENT_LATCHES.MAR);
    printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    printf("Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	printf("%d: 0x%04x\n", k, CURRENT_LATCHES.REGS[k]);
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count  : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file, "PC           : 0x%04x\n", CURRENT_LATCHES.PC);
    fprintf(dumpsim_file, "IR           : 0x%04x\n", CURRENT_LATCHES.IR);
    fprintf(dumpsim_file, "STATE_NUMBER : 0x%04x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    fprintf(dumpsim_file, "BUS          : 0x%04x\n", BUS);
    fprintf(dumpsim_file, "MDR          : 0x%04x\n", CURRENT_LATCHES.MDR);
    fprintf(dumpsim_file, "MAR          : 0x%04x\n", CURRENT_LATCHES.MAR);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	fprintf(dumpsim_file, "%d: 0x%04x\n", k, CURRENT_LATCHES.REGS[k]);
    fprintf(dumpsim_file, "\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : get_command                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */  
/*                                                             */
/***************************************************************/
void get_command(FILE * dumpsim_file) {                         
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3b-SIM> ");

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
/* Procedure : init_control_store                              */
/*                                                             */
/* Purpose   : Load microprogram into control store ROM        */ 
/*                                                             */
/***************************************************************/
void init_control_store(char *ucode_filename) {                 
    FILE *ucode;
    int i, j, index;
    char line[200];

    printf("Loading Control Store from file: %s\n", ucode_filename);

    /* Open the micro-code file. */
    if ((ucode = fopen(ucode_filename, "r")) == NULL) {
	printf("Error: Can't open micro-code file %s\n", ucode_filename);
	exit(-1);
    }

    /* Read a line for each row in the control store. */
    for(i = 0; i < CONTROL_STORE_ROWS; i++) {
	if (fscanf(ucode, "%[^\n]\n", line) == EOF) {
	    printf("Error: Too few lines (%d) in micro-code file: %s\n",
		   i, ucode_filename);
	    exit(-1);
	}

	/* Put in bits one at a time. */
	index = 0;

	for (j = 0; j < CONTROL_STORE_BITS; j++) {
	    /* Needs to find enough bits in line. */
	    if (line[index] == '\0') {
		printf("Error: Too few control bits in micro-code file: %s\nLine: %d\n",
		       ucode_filename, i);
		exit(-1);
	    }
	    if (line[index] != '0' && line[index] != '1') {
		printf("Error: Unknown value in micro-code file: %s\nLine: %d, Bit: %d\n",
		       ucode_filename, i, j);
		exit(-1);
	    }

	    /* Set the bit in the Control Store. */
	    CONTROL_STORE[i][j] = (line[index] == '0') ? 0:1;
	    index++;
	}

	/* Warn about extra bits in line. */
	if (line[index] != '\0')
	    printf("Warning: Extra bit(s) in control store file %s. Line: %d\n",
		   ucode_filename, i);
    }
    printf("\n");
}

/************************************************************/
/*                                                          */
/* Procedure : init_memory                                  */
/*                                                          */
/* Purpose   : Zero out the memory array                    */
/*                                                          */
/************************************************************/
void init_memory() {                                           
    int i;

    for (i=0; i < WORDS_IN_MEM; i++) {
	MEMORY[i][0] = 0;
	MEMORY[i][1] = 0;
    }
}

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename) {                   
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
	program_base = word >> 1;
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
	MEMORY[program_base + ii][0] = word & 0x00FF;
	MEMORY[program_base + ii][1] = (word >> 8) & 0x00FF;
	ii++;
    }

    if (CURRENT_LATCHES.PC == 0) CURRENT_LATCHES.PC = (program_base << 1);

    printf("Read %d words from program into memory.\n\n", ii);
}

/***************************************************************/
/*                                                             */
/* Procedure : initialize                                      */
/*                                                             */
/* Purpose   : Load microprogram and machine language program  */ 
/*             and set up initial state of the machine.        */
/*                                                             */
/***************************************************************/
void initialize(char *ucode_filename, char *program_filename, int num_prog_files) { 
    int i;
    init_control_store(ucode_filename);

    init_memory();
    for ( i = 0; i < num_prog_files; i++ ) {
	load_program(program_filename);
	while(*program_filename++ != '\0');
    }
    CURRENT_LATCHES.Z = 1;

    #ifdef INIT_REGS
    CURRENT_LATCHES.REGS[0] = 0;
    CURRENT_LATCHES.REGS[1] = 1;
    CURRENT_LATCHES.REGS[2] = 2;
    CURRENT_LATCHES.REGS[3] = 3;
    CURRENT_LATCHES.REGS[4] = 4;
    CURRENT_LATCHES.REGS[5] = 5;
    CURRENT_LATCHES.REGS[6] = 0x1236;
    CURRENT_LATCHES.REGS[7] = 0xABCD;
    #endif
    
    CURRENT_LATCHES.STATE_NUMBER = INITIAL_STATE_NUMBER;
    memcpy(CURRENT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[INITIAL_STATE_NUMBER], sizeof(int)*CONTROL_STORE_BITS);

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
    if (argc < 3) {
	printf("Error: usage: %s <micro_code_file> <program_file_1> <program_file_2> ...\n",
	       argv[0]);
	exit(1);
    }

    printf("LC-3b Simulator\n\n");

    initialize(argv[1], argv[2], argc - 2);

    if ( (dumpsim_file = fopen( "dumpsim", "w" )) == NULL ) {
	printf("Error: Can't open dumpsim file\n");
	exit(-1);
    }

    while (1)
	get_command(dumpsim_file);

}

/***************************************************************/
/* --------- DO NOT MODIFY THE CODE ABOVE THIS LINE -----------*/
/***************************************************************/

/***************************************************************/
/* You are allowed to use the following global variables in your
   code. These are defined above.

   CONTROL_STORE
   MEMORY
   BUS

   CURRENT_LATCHES
   NEXT_LATCHES

   You may define your own local/global variables and functions.
   You may use the functions to get at the control bits defined
   above.

   Begin your code here 	  			       */
/***************************************************************/


int memCycleCount = 0; /* initialize a global cycle coutner we can use to keep track of cycles*/
int mdrData = 0;
int sReg1 = 0;
int sReg2 = 0;
int valPC = 0;
int gateMarMux = 0;
int gateMdrMux = 0;
int gateALU = 0;
int gateSHF = 0;
int addr1Mux = 0;
int addr2Mux = 0;
int addrVal = 0;
int drMux = 0;
int tempMDR = 0;
int sReg1mux = 0;
int oddOReven = 0;



int LSHF(int value, int amount){
  return (value << amount) & 0xFFFF;
}
 

int RSHF(int value, int amount, int topbit ){
  int mask;
  mask = 1 << amount;
  mask -= 1;
  mask = mask << ( 16 -amount );
  
  return ((value >> amount) & ~mask) | ((topbit)?(mask):0); /* TBD */
}


int SEXT(int value, int topbit){
  int shift = sizeof(int)*8 - topbit; 
  return (value << shift )>> shift;
}


int ZEXT(int value, int topbit){
  return (value & ((1 << (topbit+1)) - 1));
}


int get_bits(int value, int start, int end){
  int result;
  assert(start >= end );
  result = value >> end;
  result = result % ( 1 << ( start - end + 1 ) );
  return result; 
}


int read_word(int addr){
  assert( (addr & 0xFFFF0000) == 0 );
  return MEMORY[addr>>1][1]<<8 | MEMORY[addr>>1][0];
}


int read_byte(int addr){
  int bank=addr&1;
  assert( (addr & 0xFFFF0000) == 0 );
  return MEMORY[addr>>1][bank];
}


void write_byte(int addr, int value){
  int bank=addr&1;
  assert( (addr & 0xFFFF0000) == 0 );
  MEMORY[addr>>1][bank]= value & 0xFF;
}


void write_word(int addr, int value){
  assert( (addr & 0xFFFF0000) == 0 );
  MEMORY[addr>>1][1] = (value & 0x0000FF00) >> 8;
  MEMORY[addr>>1][0] = value & 0xFF;
} 


int next_state(){
    int ready  = (!CURRENT_LATCHES.MICROINSTRUCTION[COND1]) & (CURRENT_LATCHES.MICROINSTRUCTION[COND0]) & (CURRENT_LATCHES.READY);
    int branch = (CURRENT_LATCHES.MICROINSTRUCTION[COND1]) & (!CURRENT_LATCHES.MICROINSTRUCTION[COND0]) & (CURRENT_LATCHES.BEN);
    int irBit11 = get_bits(CURRENT_LATCHES.IR, 11, 11);
    int addr_Mode = (CURRENT_LATCHES.MICROINSTRUCTION[COND1]) & (CURRENT_LATCHES.MICROINSTRUCTION[COND0]) & (irBit11);
    return (GetJ(CURRENT_LATCHES.MICROINSTRUCTION)) | (branch << 2) | (ready << 1) | (addr_Mode);
}


void eval_micro_sequencer() {

  /* 
   * Evaluate the address of the next state according to the 
   * micro sequencer logic. Latch the next microinstruction.
   */

   /* check IRD (the first bit) tells you whether to check to use opcode or Jbits */
   if(GetIRD(CURRENT_LATCHES.MICROINSTRUCTION) == 0) {
    /* next state is jbits */
    NEXT_LATCHES.STATE_NUMBER = next_state();
    memcpy(NEXT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[NEXT_LATCHES.STATE_NUMBER], sizeof(int)*CONTROL_STORE_BITS);

   } else {
    /* next state is 00 + IR{15:11]} */
    NEXT_LATCHES.STATE_NUMBER = 0x0F & (get_bits(CURRENT_LATCHES.IR, 15, 12)); 
    memcpy(NEXT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[NEXT_LATCHES.STATE_NUMBER], sizeof(int)*CONTROL_STORE_BITS);

   }

}


void cycle_memory() {

  /* 
   * This function emulates memory and the WE logic. 
   * Keep track of which cycle of MEMEN we are dealing with.  
   * If fourth, we need to latch Ready bit at the end of 
   * cycle to prepare microsequencer for the fifth cycle.  
   */
    if (GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION)) {

        if (memCycleCount == 3) {

            NEXT_LATCHES.READY = 1;

        } else if (memCycleCount == 4) {

            NEXT_LATCHES.READY = 1;

            if (GetR_W(CURRENT_LATCHES.MICROINSTRUCTION)) {

		          NEXT_LATCHES.READY = 1;

                if (CURRENT_LATCHES.MICROINSTRUCTION[DATA_SIZE]){

                   /* MEMORY[(get_bits(CURRENT_LATCHES.MAR, 15, 1))][0] = get_bits(CURRENT_LATCHES.MDR, 7, 0);
                    MEMORY[(get_bits(CURRENT_LATCHES.MAR, 15, 1))][1] = get_bits(CURRENT_LATCHES.MDR, 15, 8); */
                    write_word(CURRENT_LATCHES.MAR, (get_bits(CURRENT_LATCHES.MDR, 15, 0)));
                    NEXT_LATCHES.READY = 1;
                    memCycleCount = 0;

                } else {

                    /* MEMORY[get_bits(CURRENT_LATCHES.MAR, 15, 1)][get_bits(CURRENT_LATCHES.MAR, 0, 0)] = get_bits(CURRENT_LATCHES.MDR, 7, 0); */
                   
			
		    write_byte(CURRENT_LATCHES.MAR, (get_bits(CURRENT_LATCHES.MDR, 7, 0)));
                    NEXT_LATCHES.READY = 1;
                    memCycleCount = 0;

                }

            } else {

                    mdrData = Low16bits(MEMORY[(get_bits(CURRENT_LATCHES.MAR, 15, 1))][0] + LSHF(MEMORY[get_bits(CURRENT_LATCHES.MAR, 15, 1)][1], 8));
                    NEXT_LATCHES.READY = 1;
                    memCycleCount = 0;

            }
        }   

        memCycleCount++;

    } else {        

        memCycleCount = 0;
        NEXT_LATCHES.READY = 0;

    }

}


void eval_bus_drivers() {

  /* 
   * Datapath routine emulating operations before driving the bus.
   * Evaluate the input of tristate drivers 
   *         Gate_MARMUX,
   *		 Gate_PC,
   *		 Gate_ALU,
   *		 Gate_SHF,
   *		 Gate_MDR.
   */    

   if (GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION)) {

        sReg1 = get_bits(CURRENT_LATCHES.IR, 8, 6);

   } else {

       sReg1 = get_bits(CURRENT_LATCHES.IR, 11, 9);

   }

   sReg1mux = CURRENT_LATCHES.REGS[sReg1];

 	if ((GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION))) { 

		addr1Mux = Low16bits(sReg1mux);

	} else { 

		addr1Mux = Low16bits(CURRENT_LATCHES.PC); 

	}

   switch (GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION)) {

    case 0: addr2Mux = 0x0000;
    break;

    case 1: addr2Mux = Low16bits(SEXT(get_bits(CURRENT_LATCHES.IR, 5, 0), 6));
    break;

    case 2: addr2Mux = Low16bits(SEXT(get_bits(CURRENT_LATCHES.IR, 8, 0), 9));
    break;

    case 3: addr2Mux = Low16bits(SEXT(get_bits(CURRENT_LATCHES.IR, 10, 0), 11));
    break;

    default: exit(-1);

   }


   /* Adder */   
   if ((CURRENT_LATCHES.MICROINSTRUCTION[LSHF1])) {
	
	addrVal = Low16bits(addr1Mux + LSHF(addr2Mux, 1));

   } else { 

	addrVal = Low16bits(addr1Mux + addr2Mux); 

   }


   if (get_bits(CURRENT_LATCHES.IR, 5, 5)) {

       sReg2 = Low16bits(SEXT(get_bits(CURRENT_LATCHES.IR, 4, 0), 5));      

   } else {

        sReg2 = CURRENT_LATCHES.REGS[get_bits(CURRENT_LATCHES.IR, 2, 0)];
   }

   /* Gate ALU */

   switch (GetALUK(CURRENT_LATCHES.MICROINSTRUCTION)) {
    case 0: gateALU = Low16bits(sReg1mux + sReg2);
    break;

    case 1: gateALU = Low16bits(sReg1mux & sReg2);
    break; 

    case 2: gateALU = Low16bits(sReg1mux ^ sReg2);
    break;

    case 3: gateALU = sReg1mux;
    break;

    default: exit(1);
   }

   /* Gate SHF */

   if ((get_bits(CURRENT_LATCHES.IR, 4, 4)) == 0) {

    gateSHF = Low16bits(LSHF(sReg1mux, get_bits(CURRENT_LATCHES.IR, 3, 0)));

   } else {

        if ((get_bits(CURRENT_LATCHES.IR, 5, 5)) == 0) {

        gateSHF = Low16bits(RSHF(sReg1mux, get_bits(CURRENT_LATCHES.IR, 3, 0), 0));

        } else {

        gateSHF = Low16bits(RSHF(sReg1mux, get_bits(CURRENT_LATCHES.IR, 3, 0), (get_bits(sReg1mux, 15, 15))));

        }

   }

   /* Gate MARMUX */

   if (GetMARMUX(CURRENT_LATCHES.MICROINSTRUCTION)) {

        gateMarMux = addrVal;

   } else {

        gateMarMux = Low16bits(LSHF(ZEXT(get_bits(CURRENT_LATCHES.IR, 7, 0), 7), 1));
   }

   /* Gate PCMUX */

	valPC = CURRENT_LATCHES.PC;

   /* Gate MDR */
      
   oddOReven = get_bits(CURRENT_LATCHES.MAR, 0, 0); 
   if (GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION)) {
        
       tempMDR = mdrData;

   } else {
	
	if (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)) {

		tempMDR = BUS;

	} else {

		tempMDR = Low16bits((get_bits(BUS, 7, 0), 8)); 	

	}
   }		

}


void drive_bus() {

/* switch case for control lines that assign the bus. */
  /* 
   * Datapath routine for driving the bus from one of the 5 possible 
   * tristate drivers. 
   */       

   if (GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION)) {

        BUS = gateMarMux;

   } else if (GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION)) {

        BUS = CURRENT_LATCHES.PC;

   } else if (GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION)) {

        if (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)) {

            BUS = CURRENT_LATCHES.MDR;

        } else {
		
            if (oddOReven) {

                BUS = Low16bits(SEXT(get_bits(CURRENT_LATCHES.MDR, 15, 8), 8));
	    
	    } else {
		
		BUS = Low16bits(SEXT(get_bits(CURRENT_LATCHES.MDR, 7, 0), 8));

	    }

        }

   } else if (GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION)) {

        BUS = gateALU;

   } else if (GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION)) {

        BUS = gateSHF;

   } else {

	 BUS = 0; 
   
   }

}


void latch_datapath_values() {

  /* 
   * Datapath routine for computing all functions that need to latch
   * values in the data path at the end of this cycle.  Some values
   * require sourcing the bus; therefore, this routine has to come 
   * after drive_bus.
   */       

   if (GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION)) {

        drMux = 7;

   } else {

        drMux = get_bits(CURRENT_LATCHES.IR, 11, 9);

   }

   if (GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION)) {

        NEXT_LATCHES.REGS[drMux] = BUS;

   }

   if (GetLD_BEN(CURRENT_LATCHES.MICROINSTRUCTION)) {

	int bit11 = get_bits(CURRENT_LATCHES.IR, 11, 11);
	int bit10 = get_bits(CURRENT_LATCHES.IR, 10, 10);
	int bit9 = get_bits(CURRENT_LATCHES.IR, 9, 9);

        NEXT_LATCHES.BEN = (bit11 & CURRENT_LATCHES.N) | (bit10 & CURRENT_LATCHES.Z) | (bit9 & CURRENT_LATCHES.P);

   }

   if (GetLD_IR(CURRENT_LATCHES.MICROINSTRUCTION)) {

        NEXT_LATCHES.IR = BUS;

   }

   if (GetLD_MAR(CURRENT_LATCHES.MICROINSTRUCTION)) {

        NEXT_LATCHES.MAR = BUS;

   }

   if (GetLD_MDR(CURRENT_LATCHES.MICROINSTRUCTION)) {

       if (GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION)) {

            NEXT_LATCHES.MDR = mdrData;

       } else {

            if (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)) {

                NEXT_LATCHES.MDR = get_bits(BUS, 15, 0);

            } else {	

	        NEXT_LATCHES.MDR = get_bits(BUS, 7, 0) + (get_bits(BUS, 7, 0) << 8 );

            }

       }
	
   }

   if (GetLD_PC(CURRENT_LATCHES.MICROINSTRUCTION)) {

   	if (GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 0) {

        	NEXT_LATCHES.PC = CURRENT_LATCHES.PC + 2;

   	} else if (GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 1) {

        	NEXT_LATCHES.PC = BUS;

   	} else if (GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 2) {

        	NEXT_LATCHES.PC = addrVal;
        }

   } 
 
   if (GetLD_CC(CURRENT_LATCHES.MICROINSTRUCTION)) {

        NEXT_LATCHES.P = 0;
        NEXT_LATCHES.N = 0;
        NEXT_LATCHES.Z = 0;

        if (Low16bits(BUS) == 0) {

            NEXT_LATCHES.Z = 1;

        } else if ((BUS & 0x8000)) {

            NEXT_LATCHES.N = 1;

        } else {

            NEXT_LATCHES.P = 1;

        }

   }

}
