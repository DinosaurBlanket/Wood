#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "buf.h"

#define numType '.'
#define narType ':'
#define parType '|'
#define anyType '@'
#define dubBufChar '~'
#define cmntStart '('
#define cmntEnd   ')'
#define tokSep    '\n' // token separator
#define tokSepStr "\n"

#define _ShouldNotBeHere_ printf("Should Not Be Here: line %i of %s\n", __LINE__, __FILE__);
#define fr(i, bound) for (int i = 0; i < (bound); i++)

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
	ct_any
} cscdType;

typedef union {
	double num;
	double _unusedfornow[4];
} anyBox;

typedef struct {
	char     *name;  // includes parameters, doesn't include type chars or "fn "
	uint8_t  *types; // outType first, then params if any
	uint32_t  paramCount;
	void     *fn;    // will hold function pointers of many types, or NULL
	anyBox    value; // for nodes that output constant, used when fn == NULL
} astNodeDef;

double sl_add(double a, double b) {return a + b;}
double sl_sub(double a, double b) {return a - b;}
double sl_mul(double a, double b) {return a * b;}
double sl_div(double a, double b) {return a / b;}
uint8_t ctp_num[]       = {ct_num};
// uint8_t ctp_numnum[]    = {ct_num, ct_num};
uint8_t ctp_numnumnum[] = {ct_num, ct_num, ct_num};

#define cscdStdLibCount 5
const astNodeDef cscdStdLib[cscdStdLibCount] = {
	{
		.name       = "+" tokSepStr "a" tokSepStr "b",
		.types      = ctp_numnumnum,
		.paramCount = 2,
		.fn         = sl_add,
		.value      = 0
	},{
		.name       = "-" tokSepStr "a" tokSepStr "b",
		.types      = ctp_numnumnum,
		.paramCount = 2,
		.fn         = sl_sub,
		.value      = 0
	},{
		.name       = "*" tokSepStr "a" tokSepStr "b",
		.types      = ctp_numnumnum,
		.paramCount = 2,
		.fn         = sl_mul,
		.value      = 0
	},{
		.name       = "/" tokSepStr "a" tokSepStr "b",
		.types      = ctp_numnumnum,
		.paramCount = 2,
		.fn         = sl_div,
		.value      = 0
	},{ // this one's just a test
		.name       = "nine",
		.types      = ctp_num,
		.paramCount = 0,
		.fn         = NULL,
		.value      = 9
	}
};

typedef struct {
	astNodeDef      def;
	struct astNode *parent;   // in astNodeBuf
	uint32_t        kidCount; // incremented as parameters are filled
} astNode;

typedef struct {
	char    *name;
	uint8_t  type;
	astNode *init;
	astNode *reev;
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
	ttr_trunk,
	ttr_branch,
	ttr_fnParams,
	ttr_narLit,
	ttr_parLit
} thingsToRead;

void lookupToken(char *token, struct astNode *parent, astNode *node) {
	if (
		('0' <= *token && *token <= '9')  ||  
		(*token == '-' && '0' <= token[1] && token[1] <= '9')
	) {
		// numlit
		printUpTo(token, tokSep);
		puts("  (numlit in lookupToken)");
		return;
	}
	fr (i, cscdStdLibCount) {
		if (matchUpTo(&cscdStdLib[i].name[0], token, tokSep)) {
			//match
			printUpTo(token, tokSep);
			puts("  (StdLib match in lookupToken)");
			node->def      = cscdStdLib[i];
			node->parent   = parent;
			node->kidCount = 0;
			return;
		}
	}
	// not found
	_ShouldNotBeHere_;
}

thingsToRead handleBranchNode(char *token, astNode *pivot) {
	printUpTo(token, tokSep);
	puts("  (in handleBranchNode)");
	return ttr_branch;
}

void handleToken(char *token) {
	static astNode     *pivot   = NULL; // latest unfilled node
	static thingsToRead reading = ttr_new;
	switch (reading) {
		case ttr_new:
			switch (*token) {
				typeCharCase:
					pushEmpty_astRootBuf(&astRoots);
					plast_astRootBuf(astRoots)->name = token[1] == dubBufChar ? &token[2] : &token[1];
					switch (*token) {
						case numType: plast_astRootBuf(astRoots)->type = ct_num; break;
						case parType: plast_astRootBuf(astRoots)->type = token[1] == dubBufChar ? ct_dnar : ct_nar; break;
						case narType: plast_astRootBuf(astRoots)->type = token[1] == dubBufChar ? ct_dpar : ct_par; break;
						case anyType: plast_astRootBuf(astRoots)->type = ct_any; break;
					}
					pushEmpty_astNodeBuf(&astNodes);
					plast_astRootBuf(astRoots)->init = pivot = plast_astNodeBuf(astNodes);
					reading = ttr_trunk;
					break;
				default: _ShouldNotBeHere_;
			}
			break;
		case ttr_trunk:
			lookupToken(token, NULL, pivot);
			reading = pivot->def.paramCount ? ttr_branch : ttr_new;
			break;
		case ttr_branch:
			reading = handleBranchNode(token, pivot);
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
