#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "buf.h"

#define numType '.'
#define narType ':'
#define parType '|'
#define anyType '&'
#define cmntStart '('
#define cmntEnd   ')'
#define tokSep    '\n'
#define tokSepStr "\n"
#define rsrvd_

#define _ShouldNotBeHere_ printf("Should Not Be Here: line %i of file%s", __LINE__, __FILE__);

void printUpTo(const char *toPrint, char upTo) {
	for (
		uint32_t i = 0;
		toPrint[i] && toPrint[i] != upTo;
		i++
	) putc(toPrint[i], stdout);
}
bool matchUpTo(const char *l, const char *r, char upTo) {
	uint32_t i = 0;
	for (; l[i]  &&  r[i]  &&  l[i] != upTo  &&  r[i] != upTo; i++) {
		if (l[i] != r[i]) return false;
	}
	if ((!l[i] || l[i] == upTo) && (!r[i] || r[i] == upTo)) return true;
	return false;
}


typedef enum {
	ct_num,  // number
	ct_nar,  // number array
	ct_par,  // packed array
	ct_dnar, // number array, double-buffered
	ct_dpar, // packed array, double-buffered
	dt_any
} cscdType;

typedef union {
	double num;
	double _unusedfornow[4];
} anyBox;

typedef struct {
	char     *name;  // includes parameters, doesn't include "fn "
	uint8_t  *types; // outType first, then params if any
	uint32_t  paramCount;
	void     *fn;    // will hold function pointers of many types, or NULL
	anyBox    value; // for nodes that output constant, used when fn == NULL
} astNodeDef;

double sl_add(double a, double b) {return a + b;}
double sl_sub(double a, double b) {return a - b;}
double sl_mul(double a, double b) {return a * b;}
double sl_div(double a, double b) {return a / b;}

#define cscdStdLibCount 4
const astNodeDef cscdStdLib[cscdStdLibCount] = {
	{.name = ".+"tokSepStr".a"tokSepStr".b", .fn = sl_add, .value = 0},
	{.name = ".-"tokSepStr".a"tokSepStr".b", .fn = sl_sub, .value = 0},
	{.name = ".*"tokSepStr".a"tokSepStr".b", .fn = sl_mul, .value = 0},
	{.name = "./"tokSepStr".a"tokSepStr".b", .fn = sl_div, .value = 0}
};

typedef struct {
	char           *name; // same as astNodeDef
	struct astNode *parent;
	uint32_t        kidCount; // incremented as parameters are filled
} astNode;

typedef struct {
	char    *name;
	astNode  branch;
} astRoot;


#define SpaceCase case '\n': case '\t': case ' '
#define typeCharCase \
case numType:\
case narType:\
case parType:\
case anyType

BufType(char,    charBuf);
BufType(uint8_t, u8Buf);
BufType(astNode, astNodeBuf);
BufType(astRoot, astRootBuf);

astRootBuf astRoots;
astNodeBuf astNodes;
u8Buf      astTypes;


typedef enum {
	ttr_new,
	ttr_branch,
	ttr_fnParams,
	ttr_narLit,
	ttr_parLit
} thingsToRead;

thingsToRead handleBranchNode(char *token) {
	printUpTo(token, tokSep);
	puts("  in handleBranchNode");
	return ttr_branch;
}

void handleToken(char *token) {
	//static astNode *parent = NULL;
	static thingsToRead reading = ttr_new;
	switch (reading) {
		case ttr_new:
			switch (*token) {
				typeCharCase:
					pushEmpty_astRootBuf(&astRoots);
					plast_astRootBuf(astRoots)->name = token;
					pushEmpty_astNodeBuf(&astNodes);
					reading = ttr_branch;
					break;
				default: _ShouldNotBeHere_;
			}
			break;
		case ttr_branch:
			reading = handleBranchNode(token);
			break;
		case ttr_fnParams:
		case ttr_narLit:
		case ttr_parLit:
		default: _ShouldNotBeHere_;
	}
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
	uint32_t curLine      = 0;
	uint32_t commentDepth = 0;
	astRoots = init_astRootBuf(16);
	astNodes = init_astNodeBuf(16);
	astTypes = init_u8Buf(16);
	charBuf tokens = init_charBuf(128);
	push_charBuf(&tokens, tokSep); // to look back on without going OOB
	char *tokenStart = tokens.data + 1;
	for (
		char inchar = fgetc(infile);
		inchar != EOF;
		inchar = fgetc(infile)
	) {
		if (inchar == '\n') curLine++;
		if (commentDepth) {
			switch(inchar) {
				case cmntStart: commentDepth++; break;
				case cmntEnd  : commentDepth--; break;
			}
			continue;
		}
		switch (inchar) {
			case cmntStart: commentDepth++; // fall
			SpaceCase:
				switch (last_charBuf(tokens)) {SpaceCase: continue;}
				push_charBuf(&tokens, tokSep);
				handleToken(tokenStart);
				tokenStart = &tokens.data[tokens.count];
				continue;
		}
		push_charBuf(&tokens, inchar);
	}
	if (commentDepth) puts("Warning: unclosed comment\n");
	fclose(infile);
	if (matchUpTo(astRoots.data[0].name, ".hyaku", '\n')) puts("success");
	free(astRoots.data);
	free(astNodes.data);
	free(astTypes.data);
	free(tokens.data);
	return 0;
}
