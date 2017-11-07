// Author: Shikang Xu; ECE 353 TA
//Timothy Gerstel, Jennifer Feng, Jonathan A.
// List the full names of ALL group members at the top of your code.
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>
//feel free to add here any additional library names you may need 
#define SINGLE 1
#define BATCH 0
#define REG_NUM 32
#define MEM 512

typedef enum {
	add, addi, sub, mul, beq, lw, sw, comment
} Opcode;

struct Instr{
	Opcode op;
	int rs;
	int rt;
	int rd;
	int imm;
};

struct Reg {
	int value;
	bool wait;
};

struct Reg regs[REG_NUM];
struct Instr InstrMem[MEM];
unsigned int dataMem[MEM];

//*** Stage functions
bool IF();
bool ID();
bool EX();
bool M();
bool WB();

char* progScanner(char*);
char** regNumberConverter(char*);
char* rncHelper(char*);
struct Instr *parser(char**);
void printAndWait();

double ifUtil, idUtil, exUtil, memUtil, wbUtil;

int main (int argc, char *argv[]){
	int sim_mode=0;//mode flag, 1 for single-cycle, 0 for batch
	int c,m,n;
	int i;//for loop counter
	long mips_reg[REG_NUM];
	long pgm_c=0;//program counter
	long sim_cycle=0;//simulation cycle counter
	//define your own counter for the usage of each pipeline stage here
	
	int test_counter=0;
	FILE *input=NULL;
	FILE *output=NULL;
	printf("The arguments are:");
	
	for(i=1;i<argc;i++){
		printf("%s ",argv[i]);
	}
	printf("\n");
	if(argc==7){
		if(strcmp("-s",argv[1])==0){
			sim_mode=SINGLE;
		}
		else if(strcmp("-b",argv[1])==0){
			sim_mode=BATCH;
		}
		else{
			printf("Wrong sim mode chosen\n");
			exit(0);
		}
		
		m=atoi(argv[2]);
		n=atoi(argv[3]);
		c=atoi(argv[4]);
		input=fopen(argv[5],"r");
		output=fopen(argv[6],"w");
		
	}
	
	else{
		printf("Usage: ./sim-mips -s m n c input_name output_name (single-sysle mode)\n or \n ./sim-mips -b m n c input_name  output_name(batch mode)\n");
		printf("m,n,c stand for number of cycles needed by multiplication, other operation, and memory access, respectively\n");
		exit(0);
	}
	if(input==NULL){
		printf("Unable to open input or output file\n");
		exit(0);
	}
	if(output==NULL){
		printf("Cannot create output file\n");
		exit(0);
	}
	//initialize registers and program counter
	if(sim_mode==1){
		for (i=0;i<REG_NUM;i++){
			mips_reg[i]=0;
		}
	}
	
	// Code by Timothy Gerstel, Jennifer Feng, and Jonathan A
	for(i = 0; i < REG_NUM; i++){
		regs[i].value = 0; //Initialize value to 0
		regs[i].wait = false; //Initialize wait (to write) flag to false
	}
	char *buffer = malloc(sizeof(char) * 128);
	while(fgets(buffer, 128, input) != NULL){
		char **data = regNumberConverter(progScanner(buffer));
		//struct Instr *ins = parser(data);

		printf("cycle: %ld ",sim_cycle);
		if(sim_mode==1){
			for (i=1;i<REG_NUM;i++){
				printf("%ld  ",mips_reg[i]);
			}
		}
		printf("%ld\n",pgm_c);
		printf("press ENTER to continue\n");
		while(getchar() != '\n');
		pgm_c+=4;
		sim_cycle+=1;
		test_counter++;
	}
	free(buffer);

	if(sim_mode==0){
		fprintf(output,"program name: %s\n",argv[5]);
		fprintf(output,"stage utilization: %f  %f  %f  %f  %f \n",
                             ifUtil, idUtil, exUtil, memUtil, wbUtil);
                     // add the (double) stage_counter/sim_cycle for each 
                     // stage following sequence IF ID EX MEM WB
		
		fprintf(output,"register values ");
		for (i=1;i<REG_NUM;i++){
			fprintf(output,"%ld  ",mips_reg[i]);
		}
		fprintf(output,"%ld\n",pgm_c);
	
	}
	//close input and output files at the end of the simulation
	fclose(input);
	fclose(output);
	return 0;
}

char* progScanner(char *instr){
	int i, j=0; //Loop counters
	int delCount = 0, whitespace = 0; //Misc counters
	char buffer[32]; //Buffer
	if(strcmp(instr, "haltSimulation") == 0){
		printf("HALTING\n");
		exit(0);
	}
	for(i = 0; instr[i] != '\0'; i++){
		if(i == 0 && instr[i] =='#'){
			return "comment";
		} else {
			if(instr[i] == ' ') whitespace++; else whitespace = 0;
			if(instr[i] == '(') delCount++;
			if(instr[i] == ')') delCount--;
			if(instr[i] == '#') break;
			if(j < 22 && instr[i] != ','){
				buffer[j] = instr[i];
				if(whitespace > 2){
					buffer[j-1] = '\0';
				} else 
				if(whitespace < 2){
					j++;
				}
			}
		}
	}
	if(delCount != 0){
		printf("Delimiter mismatch");
		exit(0);
	}
	//printf("%s\n", buffer);
	return buffer;
}

char** regNumberConverter(char* line){
	int i = 0;
	char *cpy = malloc(sizeof(char) * 32);
	char *buffer[5];
	strncpy(cpy, line, 32);
	printf("%s\n", cpy);
	if(strcmp(cpy, "comment") == 0){
		printf("regNumberConverter(): Comment detected\n");
		buffer[0] = "comment";
		return buffer;
	} else {
		buffer[0] = strtok(cpy, " ()");
		while(buffer[i] != NULL && i < 5){
			//char *token = buffer[i];
			printf("rNC(): Previous element: %s\n", buffer[i]);
			if(buffer[i][0] == '$'){
				char* replaced = rncHelper(buffer[i] + 1);
				puts(replaced);
				//printf("rNC(): Replacement: %s\n", replaced);
				//buffer[i] = rncHelper(buffer[i] + 1);
				strcpy(buffer[i], replaced);
				printf("rNC(): Current element: %s\n", buffer[i]);
			}
			buffer[++i] = strtok(NULL, " ()");
		}
	}
	printf("rNC(): Buffer:\n");
	for(i = 0; i < 5; i++){
		printf("%d\t%s\n", i, buffer[i]);
	}
	free(cpy);
	return buffer;
}

char* rncHelper(char *token){
	printf("rncHelper() arg: %s\n", token);
	int i;
	char convert[6];
	char* regNames[] = {"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};
	for(i = 0; i < 32; i++){
		if(strcmp(regNames[i], token) == 0){
			snprintf(convert, sizeof(convert), "$%d%c", i, '\0');
			break;
		}
	}
	printf("rncHelper() out: %s\n", convert);
	return convert;
}

struct Instr *parser(char **data){
	int i;
	struct Instr *ret;
	struct Instr *parsed = malloc(sizeof(struct Instr));
	for(i = 0; data[i] != NULL; i++){
		char* line = data[i];
		printf("Data %d: %s\n", i, line);
	}
	free(parsed);
	return ret;
}
