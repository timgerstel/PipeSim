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
	"comment"
};
static int opVals[]={
	0, 0x8, 0, 0, 0x4, 0x23, 0x2B,1
};

static bool fetch=false; //a totally fetch variable

typedef enum {
	    add=0, addi=0x8, sub=0, mul=0, beq=0X4, lw=0X23, sw=0X2B, comment=1
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
 	bool valid; //make sure its valid
	char funct; // instruction corresponding to op
	int rs; // instruction read 1
	int rt; // instrction read 2
	int rd; //destination reg
	int imm; //immediate value
	Opcode op; // opcode of what to carry out
 };

struct id_ex_latch{
	Opcode op;
	bool valid;
	int r1; //read data 1
	int r2; //read data 2
	int rdi; //destination address 
	int memAdd; // memory address use for lw&sw
	char aluOp; //what alu will do
	bool branch; //branch on aluO=0
};
struct ex_mem_latch{
	bool valid;
 	int out; //alu output
 	bool read;//will it read if not write
	bool branch; //if branching
	int *wb; //where to write back
	Opcode op;
	int memAdd; // memory address use for lw&sw
};
struct mem_wb_latch{
Opcode op;
int memAdd; // memory address use for lw&sw
int rd; // read data
int rd2; //read data 2
int *wb; //where to write back
bool write;
};

struct if_id_latch *if_id;
struct id_ex_latch *id_ex;
struct ex_mem_latch *ex_mem;
struct mem_wb_latch *mem_wb;

struct Reg {
	int value;
	bool wait;
};

int regs[REG_NUM];
int program_counter=0;
struct Instr InstrMem[MEM];
unsigned int dataMem[MEM];

//*** Stage functions
void IF();
void ID();
void EX();
void M();
void WB();

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

double ifUtil, idUtil, exUtil, memUtil, wbUtil;

int main (int argc, char *argv[]){
	printf("blue baby blue\n");
	// LAT
// struct if_id_latch *if_id = malloc(sizeof(struct if_id_latch));
// struct id_ex_latch *id_ex = malloc(sizeof(struct id_ex_latch));
// struct ex_mem_latch *ex_mem = malloc(sizeof(struct ex_mem_latch));
// struct mem_wb_latch *mem_wb = malloc(sizeof(struct mem_wb_latch));
	int sim_mode=0;//mode flag, 1 for single-cycle, 0 for batch
	int c,m,n;
	int i;//for loop counter
	long mips_reg[REG_NUM];
	long pgm_c=0;//program counter
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
	if(sim_mode==1){
		for (i=0;i<REG_NUM;i++){
			mips_reg[i]=0;
		}
	}
	
	// Code by Timothy Gerstel, Jennifer Feng, and Jonathan A
	for(i = 0; i < REG_NUM; i++){
		regs[i] = 0; //Initialize value to 0
	//	regs[i].wait = false; //Initialize wait (to write) flag to false
	}
	char *buffer = malloc(sizeof(char) * 128);
	while(fgets(buffer, 128, input) != NULL){
		char* line = progScanner(buffer);
		//puts("SUCCESS: Scan line");
		char** data = regNumberConverter(line);
		//test(data);
		struct Instr *ins = parser(data);
		//puts("SUCCESS: Generate data array");
		//struct Instr *ins = parser(data);
		//free(line);
		printf("cycle: %ld ",sim_cycle);
		if(sim_mode==1){
			for (i=1;i<REG_NUM;i++){
				printf("%ld  ",mips_reg[i]);
			}
		}
		printf("%ld\n",pgm_c);
		printf("press ENTER to continue\n");
		fflush(stdout);

		while(getchar() != '\n');
		pgm_c+=4;
		sim_cycle+=1;
		test_counter++;
		free(data);
		//free(ins);
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
	char* buffer = malloc(sizeof(char) * 32); //Buffer
	if(strcmp(instr, "haltSimulation") == 0){
		printf("HALTING\n");
		exit(0);
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
	printf("rNC(): %s\n", line);
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
	printf("rNC(): Buffer:\n");
	for(i = 0; buffer[i] != NULL && i < 6; i++){
		printf("%d\t%s\n", i, buffer[i]);
	}
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
	printf("rncHelper() out: %s\n", convert);
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
			parsed->op = opVals[i];// if deosnt work make opvalue array to go through
			printf("parser(): opCode: %u\n", parsed->op);
			switch(parsed->op){
				case 0 :
					if(data[1][0]=='$')parsed->rd = atoi(data[1] + 1);
					if(data[2][0]=='$')parsed->rs = atoi(data[2] + 1);
					if(data[3][0]=='$')parsed->rt = atoi(data[3] + 1);
					parsed->imm = 0;
					parsed->funct=functGet(data);
					break;
				case 0x8 :
					if(data[1][0]=='$')parsed->rt = atoi(data[1] + 1);
					if(data[2][0]=='$')parsed->rs = atoi(data[2] + 1);
					parsed->imm = atoi(data[3]);
					parsed->rd = 0;
					parsed->funct =0;
					break;
				case 0x4 :
				case 0x23 :
				case 0X2B :
					if(data[1][0]=='$')parsed->rt = atoi(data[1] + 1);
					if(data[3][0]=='$')parsed->rs = atoi(data[3] + 1);
					parsed->imm = atoi(data[2]);
					//if(isdigit(atoi(data[2][0])))
					parsed->rd = 0;
					parsed->funct=0;
					break;
				case 1 :
					parsed->rd = 0;
					parsed->rs = 0;
					parsed->rt = 0;
					parsed->imm = 0;
					parsed->funct = 0;
				default:
					break;
			}
			printf("parser(): rd: %d\n", parsed->rd);
			printf("parser(): rs: %d\n", parsed->rs);
			printf("parser(): rt: %d\n", parsed->rt);
			printf("parser(): imm: %d\n", parsed->imm);
		}
		opc++;
// >>>>>>> e3be477fba19b177bfa0219583a208626c6a8de1
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
}
 // struct if_id{
	// int funct; // instruction corresponding to op
	// int rs; // instruction read 1
	// int rt; // instrction read 2
	// int rd; //destination reg
	// Opcode op; // opcode of what to carry out
 // }

void ifIdFlush(){ // flush/ reinitialize if_id after
	if_id->valid=false;
	if_id->funct=0;
	if_id->rs=0;
	if_id->rt=0;
	if_id->rd=0;
	if_id->op=1; // if op=comment same as no op
}
void IF(){ //get instruction save info to latch
if(fetch==false){
 	struct Instr instruction=InstrMem[program_counter];
 	if_id->funct=instruction.funct;
	if_id->rs=instruction.rs;
	if_id->rt=instruction.rt;
	if_id->rd=instruction.rd;
	if_id->op=instruction.op;
	if_id->imm=instruction.imm;
	if_id->valid=true;
	++program_counter;
}
}
void ID(){
if ((if_id->valid)!=true) IF();
id_ex->op=if_id->op;
if((if_id->op)==0x4){
	ifIdFlush();
	fetch=false;
	id_ex->branch=true;	
	id_ex->r1=(regs[if_id->rs]);
	id_ex->r2=(regs[if_id->rt]);
	id_ex->aluOp=0;
	id_ex->rdi=0;
	id_ex->memAdd=if_id->imm;
}
else id_ex->branch=false;

if ((if_id->op)==0)
{
	id_ex->r1=(regs[if_id->rs]);
	id_ex->r2=(regs[if_id->rt]);
	id_ex->rdi=if_id->rd;
	id_ex->aluOp=if_id->funct;
	id_ex->memAdd=0;
}
else id_ex->r2=0;

if((if_id->op)==0x8){
	id_ex->r1=(regs[if_id->rs]);
	id_ex->rdi=if_id->rt;
	id_ex->aluOp='+';
	id_ex->r2=if_id->imm;
	id_ex->memAdd=0;
}else{ 
	id_ex->aluOp=0;
	id_ex->r1=0;
}

if(((if_id->op)==0x23) ||((if_id->op)==0x2B)){
	id_ex->rdi=if_id->rt;
	id_ex->memAdd=(regs[if_id->rs])+(if_id->imm);
}
id_ex->valid=true;
}


void EX(){
int aluOutput=alu((id_ex->r1),(id_ex->r2),(id_ex->aluOp));
if ((id_ex->valid)!=true) ID();	
	ex_mem->op=id_ex->op;
if(id_ex->op=0x4){
	ex_mem->branch=true;
	ex_mem->read=0;
	ex_mem->out=0;
	ex_mem->memAdd=0;
	if(aluOutput==0){
		program_counter+=id_ex->memAdd;
	}
	return;
}else ex_mem->branch=false;

if( (id_ex->op=0) ||(id_ex->op=0x8)){
	ex_mem->memAdd=id_ex->rdi;
	ex_mem->out=aluOutput;
	ex_mem->read=0;
}

if(id_ex->op=0x23){
	ex_mem->memAdd=id_ex->memAdd;
	ex_mem->out=id_ex->rdi;
	ex_mem->read=0;
}

}


int alu(int val1, int val2,char op){
	int aluOut=0;
switch(op){
	case'+':{
		aluOut=val1+val2;
	}
	case'-':{
		aluOut=val1-val2;
	}
	case'*':{
		aluOut=val1*val2;
	}
}
return aluOut;
}