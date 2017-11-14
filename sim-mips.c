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

static char* opNames[]= {
	"add", "addi",
	"sub",
	"mul",
	"beq",
	"lw",
	"sw",
	"comment",
	"haltSimulation"
};

typedef enum {
	    add=0, addi=1, sub=2, mul=3, beq=4, lw=5, sw=6, comment=7, haltSimulation=8
/*funct: 0x20	          0x22	0x18									*/	
} Opcode;

struct Instr{
	Opcode op;
	int rs;
	int rt;
	int rd;
	int imm;
	char funct;
};
 struct if_id_latch{
	struct Instr ins;
	int cycles;
 };

struct id_ex_latch{
	Opcode op;
	int r1; //read data 1
	int r2; //read data 2
	int r_write; //write register
	int imm;
	int memAdd; // memory address use for lw&sw
	char aluOp; //what alu will do
	bool branch; //branch on aluO=0
	int cycles;
};
struct ex_mem_latch{
	Opcode op;
 	int rd;
 	int out; //alu output
	int cycles;
	bool branch; //if branching
};
struct mem_wb_latch{
	Opcode op;
	int rd; // read data
	int out;
};

struct if_id_latch *if_id;
struct id_ex_latch *id_ex;
struct ex_mem_latch *ex_mem;
struct mem_wb_latch *mem_wb;

struct Reg {
	int value;
	bool wait;
};

struct Reg mips_regs[REG_NUM];
int pgm_c = 0;
int c,m,n;
struct Instr instrMem[MEM];
unsigned int dataMem[MEM];

//*** Stage functions
bool IF(struct if_id_latch*);
bool ID(struct if_id_latch*, struct id_ex_latch*);
bool EX(struct id_ex_latch*, struct ex_mem_latch*);
bool M(struct ex_mem_latch*, struct mem_wb_latch*);
bool WB(struct mem_wb_latch*);

char* progScanner(char*);
char** regNumberConverter(char*);
char* rncHelper(char*);
struct Instr *parser(char**);
void printAndWait();
void test(char**);
int mystrcmp(char*,char*);
char functGet(char**);
void ifIdFlush();
int alu(int,int,char);
void initRegTest();

double ifUtil, idUtil, exUtil, memUtil, wbUtil;

int main (int argc, char *argv[]){
	// LAT
 	struct if_id_latch *if_id = malloc(sizeof(struct if_id_latch));
	struct id_ex_latch *id_ex = malloc(sizeof(struct id_ex_latch));
	struct ex_mem_latch *ex_mem = malloc(sizeof(struct ex_mem_latch));
	struct mem_wb_latch *mem_wb = malloc(sizeof(struct mem_wb_latch));
	int sim_mode=0;//mode flag, 1 for single-cycle, 0 for batch
	int i;//for loop counter
	long sim_cycle=0;//simulation cycle counter
	//define your own counter for the usage of each pipeline stage here
	fflush(stdout);
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
	// if(sim_mode==1){
	// 	for (i=0;i<REG_NUM;i++){
	// 		mips_reg[i]=0;
	// 	}
	// }
	
	// Code by Timothy Gerstel, Jennifer Feng, and Jonathan A
	for(i = 0; i < REG_NUM; i++){
		mips_regs[i].value = 0; //Initialize value to 0
		mips_regs[i].wait = false; //Initialize wait (to access) flag to false
	}
	char *buffer = malloc(sizeof(char) * 128);
	i = 0;
	while(fgets(buffer, 128, input) != NULL){
		char* line = progScanner(buffer);
		//puts("SUCCESS: Scan line");
		char** data = regNumberConverter(line);
		//test(data);
		struct Instr *ins = parser(data);
		instrMem[i++] = *ins;
		//puts("SUCCESS: Generate data array");
		//struct Instr *ins = parser(data);
		//free(line);
		test_counter++;
		free(data);
		//free(ins);
	}
	puts("INS MEM OPS:");
	for(i = 0; i < MEM; i++){
		printf("%d\n", instrMem[i].op);
		if(instrMem[i].op == 8){
			break;
		}
	}
	free(buffer);
	if_id->ins.op = 0;
	if_id->ins.rs = 0;
	if_id->ins.rt = 0;
	if_id->ins.rd = 0;
	if_id->ins.imm = 0;
	if_id->ins.funct = ' ';
	if_id->cycles = 0;

	id_ex->op = 0;
	id_ex->r1 = 0;
	id_ex->r2 = 0;
	id_ex->r_write = 0;
	id_ex->memAdd = 0;
	id_ex->aluOp = ' ';
	id_ex->branch = false;
	id_ex->cycles = 0;

	ex_mem->op = 0;
	ex_mem->rd = 0;
	ex_mem->out = 0;
	ex_mem->branch = false;
	ex_mem->cycles = 0;
	// ex_mem->cycles = 0;

	mem_wb->op = 0;
	mem_wb->rd = 0;
	mem_wb->out = 0;

	//initRegTest();

	while(true){
		if(WB(mem_wb)){
			if(M(ex_mem, mem_wb)){
				if(EX(id_ex, ex_mem)){
					if(ID(if_id, id_ex)){
						if(IF(if_id)){
							pgm_c += 4;
						}
					}
				}
			}
		} else {
			break;
		}
		printf("cycle: %ld ",sim_cycle);
		if(sim_mode==1){
			for (i=1;i<REG_NUM;i++){
				printf("%d  ",mips_regs[i].value);
			}
			printf("%d\n",pgm_c);
			printf("press ENTER to continue\n");
			fflush(stdout);

			while(getchar() != '\n');
		}
		sim_cycle+=1;
	}

	if(sim_mode==0){
		fprintf(output,"program name: %s\n",argv[5]);
		fprintf(output,"stage utilization: %f  %f  %f  %f  %f \n",
                             ifUtil, idUtil, exUtil, memUtil, wbUtil);
                     // add the (double) stage_counter/sim_cycle for each 
                     // stage following sequence IF ID EX MEM WB
		
		fprintf(output,"register values ");
		for (i=1;i<REG_NUM;i++){
			fprintf(output,"%d  ", mips_regs[i].value);
		}
		fprintf(output,"%d\n",pgm_c);
	
	}
	//close input and output files at the end of the simulation
	fclose(input);
	fclose(output);
	return 0;
}

void initRegTest(){
	int i;
	for(i = 0; i < REG_NUM; i++){
		mips_regs[i].value = 1;
	}
}

char* progScanner(char *instr){
	int i, j=0; //Loop counters
	int delCount = 0, whitespace = 0; //Misc counters
	char* buffer = malloc(sizeof(char) * 32); //Buffer
	if(strcmp(instr, "haltSimulation") == 0){
		return "haltSimulation\0";
	}
	for(i = 0; instr[i] != '\0'; i++){
		if(i == 0 && instr[i] =='#'){
			return "comment\0";
		} else {
			if(instr[i] == ' ') whitespace++; else whitespace = 0;
			if(instr[i] == '(') delCount++;
			if(instr[i] == ')') delCount--;
			if(instr[i] == '#') break;
			if(j < 32 && instr[i] != ','){
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
	int i;
	char** buffer = (char**) malloc(sizeof(char*) * 6);
	for(i = 0; i < 6; i++){
		buffer[i] = (char*) malloc(sizeof(char) * 8);
	}
	//printf("rNC(): %s\n", line);
	if(!strcmp(line, "haltSimulation")){
		strcpy(buffer[0], "haltSimulation");
		return buffer;
	}
	if(strcmp(line, "comment") == 0){
		//printf("rNC(): Comment detected\n");
		strcpy(buffer[0], "comment");
		return buffer;
	} else {
		i = 0;
		buffer[0] = strtok(line, " ()");
		while(buffer[i] != NULL && i < 6){
			//char *token = buffer[i];
			if(buffer[i][0] == '$'){
				//printf("rNC(): Previous element: %s\n", buffer[i]);
				char* reg = buffer[i] + 1;
				char* replaced = rncHelper(buffer[i] + 1);
				//puts(replaced);
				//printf("rNC(): Replacement: %s\n", replaced);
				//buffer[i] = rncHelper(buffer[i] + 1);
				buffer[i] = replaced;
				//printf("rNC(): Current element: %s\n", buffer[i]);
			}
			buffer[++i] = strtok(NULL, " ()");
		}
	}
	// printf("rNC(): Buffer:\n");
	// for(i = 0; buffer[i] != NULL && i < 6; i++){
	// 	printf("%d\t%s\n", i, buffer[i]);
	// }
	return buffer;
}

char* rncHelper(char *token){
	//printf("rncHelper() arg: %s\n", token);
	int i, j;
	char* convert = malloc(sizeof(char) * 6);
	char* cpy = malloc(1 + strlen(token));
	for(i = 0; i < strlen(token); i++){
		cpy[i] = token[i];
	}
	cpy[i] = '\0';
	//this is quarks fault not mine, go bark up the ECE departments tree if you want a for loop
	if(!strcmp(cpy, "zero")){
		snprintf(convert, sizeof(convert), "$%d%c", 0, '\0');
	}
	if(!strcmp(cpy, "at")){
		snprintf(convert, sizeof(convert), "$%d%c", 1, '\0');
	}
	if(!strcmp(cpy, "v0")){
		snprintf(convert, sizeof(convert), "$%d%c", 2, '\0');
	}
	if(!strcmp(cpy, "v1")){
		snprintf(convert, sizeof(convert), "$%d%c", 3, '\0');
	}
	if(!strcmp(cpy, "a0")){
		snprintf(convert, sizeof(convert), "$%d%c", 4, '\0');
	}
	if(!strcmp(cpy, "a1")){
		snprintf(convert, sizeof(convert), "$%d%c", 5, '\0');
	}
	if(!strcmp(cpy, "a2")){
		snprintf(convert, sizeof(convert), "$%d%c", 6, '\0');
	}
	if(!strcmp(cpy, "a3")){
		snprintf(convert, sizeof(convert), "$%d%c", 7, '\0');
	}
	if(!strcmp(cpy, "t0")){
		snprintf(convert, sizeof(convert), "$%d%c", 8, '\0');
	}
	if(!strcmp(cpy, "t1")){
		snprintf(convert, sizeof(convert), "$%d%c", 9, '\0');
	}
	if(!strcmp(cpy, "t2")){
		snprintf(convert, sizeof(convert), "$%d%c", 10, '\0');
	}
	if(!strcmp(cpy, "t3")){
		snprintf(convert, sizeof(convert), "$%d%c", 11, '\0');
	}
	if(!strcmp(cpy, "t4")){
		snprintf(convert, sizeof(convert), "$%d%c", 12, '\0');
	}
	if(!strcmp(cpy, "t5")){
		snprintf(convert, sizeof(convert), "$%d%c", 13, '\0');
	}
	if(!strcmp(cpy, "t6")){
		snprintf(convert, sizeof(convert), "$%d%c", 14, '\0');
	}
	if(!strcmp(cpy, "t7")){
		snprintf(convert, sizeof(convert), "$%d%c", 15, '\0');
	}
	if(!strcmp(cpy, "s0")){
		snprintf(convert, sizeof(convert), "$%d%c", 16, '\0');
	}
	if(!strcmp(cpy, "s1")){
		snprintf(convert, sizeof(convert), "$%d%c", 17, '\0');
	}
	if(!strcmp(cpy, "s2")){
		snprintf(convert, sizeof(convert), "$%d%c", 18, '\0');
	}
	if(!strcmp(cpy, "s3")){
		snprintf(convert, sizeof(convert), "$%d%c", 19, '\0');
	}
	if(!strcmp(cpy, "s4")){
		snprintf(convert, sizeof(convert), "$%d%c", 20, '\0');
	}
	if(!strcmp(cpy, "s5")){
		snprintf(convert, sizeof(convert), "$%d%c", 21, '\0');
	}
	if(!strcmp(cpy, "s6")){
		snprintf(convert, sizeof(convert), "$%d%c", 22, '\0');
	}
	if(!strcmp(cpy, "s7")){
		snprintf(convert, sizeof(convert), "$%d%c", 23, '\0');
	}
	if(!strcmp(cpy, "t8")){
		snprintf(convert, sizeof(convert), "$%d%c", 24, '\0');
	}
	if(!strcmp(cpy, "t9")){
		snprintf(convert, sizeof(convert), "$%d%c", 25, '\0');
	}
	if(!strcmp(cpy, "k0")){
		snprintf(convert, sizeof(convert), "$%d%c", 26, '\0');
	}
	if(!strcmp(cpy, "k1")){
		snprintf(convert, sizeof(convert), "$%d%c", 27, '\0');
	}
	if(!strcmp(cpy, "gp")){
		snprintf(convert, sizeof(convert), "$%d%c", 28, '\0');
	}
	if(!strcmp(cpy, "sp")){
		snprintf(convert, sizeof(convert), "$%d%c", 29, '\0');
	}
	if(!strcmp(cpy, "fp")){
		snprintf(convert, sizeof(convert), "$%d%c", 30, '\0');
	}
	if(!strcmp(cpy, "ra")){
		snprintf(convert, sizeof(convert), "$%d%c", 31, '\0');
	}
	//printf("rncHelper() out: %s\n", convert);
	free(cpy);
	return convert;
}

void test(char** data){
	int i;
	puts("Buffer read test:");
	for(i = 0; i < 6; i++){
		char *line = data[i];
		if(line != NULL && strlen(line) > 0){
			printf("%d\t%s\n", i, line);
		}
	}
}

struct Instr *parser(char **data){
	int i;
	int check =0;
	struct Instr *parsed = malloc(sizeof(struct Instr));
	if(data[0] == NULL){
		printf("parser(): Invalid data\n");
		exit(0);
	}
	Opcode opc=add;
	for(i = 0; i < sizeof(opNames)/sizeof(char*); i++){
		if(mystrcmp(data[0], opNames[i]) == 0){
			check = 1;
			parsed->op = (Opcode)i;// if deosnt work make opvalue array to go through
			//printf("parser(): opCode: %u\n", parsed->op);
			switch(parsed->op){
				case 0 :
				case 2 :
				case 3 :
					if(data[1][0]=='$')parsed->rd = atoi(data[1] + 1);
					if(data[2][0]=='$')parsed->rs = atoi(data[2] + 1);
					if(data[3][0]=='$')parsed->rt = atoi(data[3] + 1);
					parsed->imm = 0;
					parsed->funct=functGet(data);
					break;
				case 1 :
					if(data[1][0]=='$')parsed->rt = atoi(data[1] + 1);
					if(data[2][0]=='$')parsed->rs = atoi(data[2] + 1);
					parsed->imm = atoi(data[3]);
					parsed->rd = 0;
					parsed->funct = '+';
					break;
				case 4 :
				case 5 :
				case 6 :
					if(data[1][0]=='$')parsed->rt = atoi(data[1] + 1);
					if(data[3][0]=='$')parsed->rs = atoi(data[3] + 1);
					parsed->imm = atoi(data[2]);
					//if(isdigit(atoi(data[2][0])))
					parsed->rd = 0;
					parsed->funct=0;
					break;
				case 7 :
				case 8 :
					parsed->rd = 0;
					parsed->rs = 0;
					parsed->rt = 0;
					parsed->imm = 0;
					parsed->funct = 0;
					break;
				default:
					break;
			}
		}
		opc++;
	}
	if(check==0)printf("parser(): illegal opCode: %s\n",data[0]);
	return parsed;
}

int mystrcmp(char *str1,char *str2){
  while (*str1 && *str1 == *str2) {
    str1++;
    str2++;
  }
  return *str1 - *str2;
}

char functGet(char** opCode){
	char **operation = opCode;
	if(mystrcmp(operation[0],"add")==0) return '+';
	if(mystrcmp(operation[0],"sub")==0) return '-';
	if(mystrcmp(operation[0],"mult")==0) return '*';
	return ' ';
}

bool IF(struct if_id_latch *latch){ //get instruction save info to latch
	//puts("IF");
 	struct Instr instruction = instrMem[pgm_c / 4];
 	if(instruction.op == 8 || instruction.op == 7){
 		latch->ins = instruction;
 		return false;
 	}
 	ifUtil++;
 	if(latch->cycles == 0){
 		latch->cycles = c;
 	}
 	latch->cycles--;
 	if(latch->cycles == 0){
 		latch->ins = instruction;
 		printf("Current Instruction Op: %d\n", latch->ins.op);
 		return true;
 	} else {
 		latch->ins.op = 0;
 		latch->ins.rs = 0;
 		latch->ins.rt = 0;
 		latch->ins.rd = 0;
 		latch->ins.imm = 0;
 		latch->ins.funct = ' ';
	 	return false;
	}
}

bool ID(struct if_id_latch *pipe, struct id_ex_latch *latch){
	//puts("ID");
	if(pipe->ins.op == 8 || pipe->ins.op == 7){
		latch->op = pipe->ins.op;
		latch->r1 = 0;
		latch->r2 = 0;
		latch->r_write = 0;
		latch->memAdd = 0;
		return false;
	}
	idUtil++;
	bool data2;
	switch(pipe->ins.op){
		case 0 :
		case 2 :
		case 3 :
		case 4 :
			data2 = true;
			break;
		case 1 :
		case 7 :
		case 8 :
		case 5 :
		case 6 :
			data2 = false;
			break;
	}
	if(!mips_regs[pipe->ins.rs].wait){
		latch->op = pipe->ins.op;
		latch->r1 = mips_regs[pipe->ins.rs].value;
		latch->r2 = data2 ? mips_regs[pipe->ins.rt].value : 0;
		//puts("1");
		switch(latch->op){
			case 0:
			case 2:
			case 3:
				latch->r_write = pipe->ins.rd;
				latch->aluOp = pipe->ins.funct;
				latch->memAdd = 0;
				latch->imm = 0;
				break;
			case 1:
				latch->r_write = pipe->ins.rt;
				latch->imm = pipe->ins.imm;
				latch->aluOp = pipe->ins.funct;
				latch->memAdd = 0;
				break;
			case 4:
				if(latch->op == 4){
					//puts("BEQ");
					// ifIdFlush();
					latch->branch = true;
					//latch->aluOp = 0;
					latch->r_write = 0;
					latch->memAdd = pipe->ins.imm;
					//return false;
				} else { 
					//puts("NOT BEQ");
					latch->branch=false;
				}
			case 5:
				latch->r_write = pipe->ins.rt;
				latch->imm = pipe->ins.imm;
				latch->aluOp = ' ';
				latch->memAdd = pipe->ins.rs;
				break;
			case 6:
				latch->r_write = 0;
				latch->imm = pipe->ins.imm;
				latch->aluOp = ' ';
				latch->memAdd = pipe->ins.rs;
				break;
			case 7:
				latch->op = 7;
				latch->r1 = 0;
				latch->r2 = 0;
				latch->r_write = 0;
				latch->imm = 0;
				latch->memAdd = 0;
				return false;
			default:
				latch->op = 0;
				latch->r1 = 0;
				latch->r2 = 0;
				latch->r_write = 0;
				latch->imm = 0;
				latch->memAdd = 0;
				break;
		}
		if(latch->r_write != 0){
			mips_regs[latch->r_write].wait = true;
		}
		return true;
	} else {
		return false;
	}
}


bool EX(struct id_ex_latch *pipe, struct ex_mem_latch *latch){
	//puts("EX");
	if(pipe->op == 8 || pipe->op == 7){
		latch->op = pipe->op;
		latch->out = 0;
		latch->rd = 0;
		latch->branch = false;
		return false;
	}
	exUtil++;

	if(pipe->cycles == 0){
		printf("Cycle choice: %d\n", (pipe->op == 3) ? m : n);
		pipe->cycles = (pipe->op == 3) ? m : n;
	}
	pipe->cycles--;
	if(pipe->cycles == 0){
		int aluOutput = alu((pipe->r1),(pipe->r2),(pipe->aluOp));
		latch->op = pipe->op;
		switch(latch->op){
			case 0:
			case 2:
			case 3:
				latch->rd = pipe->r_write;
				latch->out = aluOutput;
				break;
			case 1:
				latch->rd = pipe->r_write;
				latch->out = alu(pipe->r1, pipe->imm, pipe->aluOp);
				break;
			case 4:
				if(latch->op == 4){
					latch->branch = true;
					latch->out = 0;
					if(aluOutput == 0){
						pgm_c+=pipe->memAdd;
					}
				}else { 
					latch->branch=false;
				}
				break;
			case 5:
			case 6:
				latch->rd = pipe->r_write;
				latch->out = pipe->memAdd + pipe->imm;
				break;
			default:
				latch->out = 0;
				latch->rd = 0;
				break;
		}
		return true;
	} else {
		latch->op = 0;
		latch->out = 0;
		latch->rd = 0;
		return false;
	}
}

bool M(struct ex_mem_latch *pipe, struct mem_wb_latch *latch){
	//puts("MEM");
	if(pipe->op == 8 || pipe->op == 7){
		latch->op = pipe->op;
		latch->rd = 0;
		latch->out = 0;
		return false;
	}
	memUtil++;
	if(pipe->cycles == 0){
		printf("Mem cycles: %d\n", (pipe->op == 5 || pipe->op == 6) ? c : 1);
		pipe->cycles = (pipe->op == 5 || pipe->op == 6) ? c : 1;
	}
	pipe->cycles--;
	if(pipe->cycles == 0){
		latch->op = pipe->op;
		switch(latch->op){
		case 5 :
			latch->out = dataMem[pipe->out / 4];
			latch->rd = pipe->rd;
			break;
		case 6 :
			dataMem[pipe->out / 4] = pipe->rd;
			latch->out = 0;
			break;
		default:
			latch->out = pipe->out;
			latch->rd = pipe->rd;
			break;
		}
		return true;
	} else {
		latch->op = 0;
		latch->rd = 0;
		latch->out = 0;
		return false;
	}
}

bool WB(struct mem_wb_latch* pipe){
	if(pipe->op == 8 || pipe->op == 7){
		return false;
	}
	wbUtil++;
	switch(pipe->op){
		case 0:
		case 1:
		case 2:
		case 3:
		case 5:
			if(pipe->rd != 0){
				printf("WRITING BACK TO $%d\n", pipe->rd);
				mips_regs[pipe->rd].value = pipe->out;
				mips_regs[pipe->rd].wait = false;
			}
			break;
		default:
			break;
	}
	return true;
}


int alu(int val1, int val2, char op){
	switch(op){
		case'+':
			return val1+val2;
			break;
		case'-':
			return val1-val2;
			break;
		case'*':
			return val1*val2;
			break;
		default:
			return 0;
			break;
	}
}