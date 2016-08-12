#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "buf.h"

#define numType  '.'
#define narType  ':'
#define parType  '|'
#define thruType '~'
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
char *tokenPastType(char *token) {
	switch (*token) {
		case numType : // fall
		case thruType: return token+1;
		case narType : return token[1] == narType ? token+2 : token+1;
		case parType : return token[1] == parType ? token+2 : token+1;
	}
	return token;
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
	uint32_t  paramCount;
	void     *fn;    // will hold function pointers of many types
} astNodeDef;

double sl_add(double a, double b) {return a + b;}
double sl_sub(double a, double b) {return a - b;}
double sl_mul(double a, double b) {return a * b;}
double sl_div(double a, double b) {return a / b;}
double sl_nine(void) {return 9;}
void   sl_root(void) {};
void   sl_numLit(void) {};

#define cscdStdLibCount 5
const astNodeDef cscdStdLib[cscdStdLibCount] = {
	{.name = ".+" tokSepStr ".a" tokSepStr ".b", .paramCount = 2, .fn = sl_add},
	{.name = ".-" tokSepStr ".a" tokSepStr ".b", .paramCount = 2, .fn = sl_sub},
	{.name = ".*" tokSepStr ".a" tokSepStr ".b", .paramCount = 2, .fn = sl_mul},
	{.name = "./" tokSepStr ".a" tokSepStr ".b", .paramCount = 2, .fn = sl_div},
	{.name = ".nine",                            .paramCount = 0, .fn = sl_nine}
};

typedef struct {
	astNodeDef  def;
	uint32_t    kidCount; // incremented as parameters are filled
	anyBox      litVal;
} astNode;

// typedef struct {
// 	char    *name;
// 	uint8_t  type;
// 	astNode *init;
// 	astNode *reev;
// } astRoot;


#define SpaceCase case '\n': case '\t': case ' '
#define typeCharCase \
case numType:\
case narType:\
case parType:\
case thruType

BufType(char,    charBuf);
BufType(astNode, astNodeBuf);

astNodeBuf astNodes;
charBuf    astTypes;

typedef enum {
	ttr_new,
	ttr_branch,
	ttr_fnParams,
	ttr_narLit,
	ttr_parLit,
	ttr_error
} thingsToRead;

thingsToRead nodeFromToken(char *token) {
	printUpTo(token, tokSep);
	printf("  (in nodeFromToken)\n");
	pushEmpty_astNodeBuf(&astNodes);
	astNode *lastNode = plast_astNodeBuf(astNodes);
	if (
		('0' <= *token && *token <= '9')  ||  
		(*token == '-' && '0' <= token[1] && token[1] <= '9')
	) {
		// numlit
		lastNode->def.name       = token;
		lastNode->def.paramCount = 0;
		lastNode->def.fn         = sl_numLit;
		lastNode->kidCount       = 0;
		lastNode->litVal.num     = 0;
		return ttr_branch;
	}
	fr (i, cscdStdLibCount) {

		if (matchUpTo(tokenPastType(&cscdStdLib[i].name[0]), token, tokSep)) {
			//match
			lastNode->def        = cscdStdLib[i];
			lastNode->kidCount   = 0;
			lastNode->litVal.num = 0;
			return ttr_branch;
		}
	}
	// not found
	_ShouldNotBeHere_;
	return ttr_error;
}
thingsToRead newFromToken(char *token) {
	printUpTo(token, tokSep);
	printf("  (in newFromToken)\n");
	astNode *lastNode;
	switch (*token) {
		typeCharCase:
			pushEmpty_astNodeBuf(&astNodes);
			lastNode = plast_astNodeBuf(astNodes);
			lastNode->def.name       = token;
			lastNode->def.paramCount = 1;
			lastNode->def.fn         = sl_root;
			lastNode->kidCount       = 0;
			lastNode->litVal.num     = 0;
			return ttr_branch;
		default: _ShouldNotBeHere_;
	}
	return ttr_error;
}

void handleToken(char *token) {
	static thingsToRead reading = ttr_new;
	switch (reading) {
		case ttr_new:    reading = newFromToken(token); break;
		case ttr_branch: reading = nodeFromToken(token); break;
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
	astNodes       = init_astNodeBuf(32);
	astTypes       = init_charBuf(32);
	charBuf tokens = init_charBuf(256);
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
	free(astNodes.data);
	free(astTypes.data);
	free(tokens.data);
	return 0;
}
