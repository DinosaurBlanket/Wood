#include <stdio.h>
#include <stdint.h>
#include "buf.h"

#define numType '.'
#define narType ':'
#define parType '|'
#define anyType '&'
#define cmntStartChar '('
#define cmntEndChar   ')'


typedef union {
	double num;
	double unusedfornow[4];
} anyBox;

typedef struct {
	char  *name;  // includes parameters, doesn't include "fn "
	void  *fn;    // will hold function pointers of many types, or NULL
	anyBox value; // for nodes that output constant, used when fn == NULL
} astNodeDef;

double sl_add(double a, double b) {return a + b;}
//const astNodeDef and_add = {".+ .a .b", sl_add, 0};
double sl_sub(double a, double b) {return a - b;}
double sl_mul(double a, double b) {return a * b;}
double sl_div(double a, double b) {return a / b;}

#define cscdStdLibCount 4
const astNodeDef cscdStdLib[cscdStdLibCount] = {
	{.name = ".+ .a .b", .fn = sl_add, .value = 0},
	{.name = ".- .a .b", .fn = sl_sub, .value = 0},
	{.name = ".* .a .b", .fn = sl_mul, .value = 0},
	{.name = "./ .a .b", .fn = sl_div, .value = 0}
};


typedef struct {
	char           *name; // same as astNodeDef
	struct astNode *kids;
	uint32_t        kidCount;
} astNode;

typedef struct {
	char    *name;
	astNode  init;
	astNode  reev;
} astRoot;

BufType(char, charBuf);
BufType(astNode, astNodeBuf);
BufType(astRoot, astRootBuf);

typedef enum {
	ttr_new,
	ttr_trunk,
	ttr_branch,
	ttr_fnParams,
	ttr_narLit,
	ttr_parLit
} thingsToRead;

void handleToken(char *token) {
	puts("yo");
}

int main(int argc, char** argv ) {
	if (argc < 2) {
		puts("usage:\ncscd HelloWorld.cscd\n");
		return 1;
	}
  char *fileName = argv[1];
	FILE *infile = fopen(fileName, "r");
	if (!infile) {
		printf("could not open '%s'\n", fileName);
		return 2;
	}
	
	uint32_t   curLine      = 0;
	uint32_t   commentDepth = 0;
	//astRootBuf roots        = init_astRoot(1);
	//astNodeBuf curRootExpr  = init_astNode(1);
	charBuf    tokens       = init_char(128);
	push_char(&tokens, ' '); // to look back on without going OOB
	char      *curToken     = NULL;
	
	for (
		char inchar = fgetc(infile);
		inchar != EOF;
		inchar = fgetc(infile)
	) {
		if (inchar == '\n') curLine++;
		if (commentDepth) {
			switch(inchar) {
				case cmntStartChar: commentDepth++; break;
				case cmntEndChar  : commentDepth--; break;
			}
			continue;
		}
		switch (inchar) {
			case cmntStartChar: commentDepth++; // fall
			case '\n': // fall
			case '\t': // fall
			case ' ' :
				switch (last_char(&tokens)) {
					case '\n': // fall
					case '\t': // fall
					case ' ' :
						continue;
				}
				handleToken(curToken);
				push_char(&tokens, '\n');
				continue;
		}
		push_char(&tokens, inchar);
		curToken = &tokens.data[tokens.count];
	}
	if (commentDepth) puts("Warning: unclosed comment\n");
	fclose(infile);
	
	puts(tokens.data);
	
	return 0;
}
