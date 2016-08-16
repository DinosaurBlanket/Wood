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

#define _ShouldNotBeHere_ printf("Should Not Be Here: line %i of %s\n", __LINE__, __FILE__)
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
	uint32_t  arity;
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
	{.name = ".+" tokSepStr ".a" tokSepStr ".b", .arity = 2, .fn = sl_add},
	{.name = ".-" tokSepStr ".a" tokSepStr ".b", .arity = 2, .fn = sl_sub},
	{.name = ".*" tokSepStr ".a" tokSepStr ".b", .arity = 2, .fn = sl_mul},
	{.name = "./" tokSepStr ".a" tokSepStr ".b", .arity = 2, .fn = sl_div},
	{.name = ".nine",                            .arity = 0, .fn = sl_nine}
};

typedef struct {
	astNodeDef def;
	uint32_t   line;     // in source file
	uint32_t   kidsIndx; // in astKids array
	uint32_t   kidCount; // incremented as arguments are added
	anyBox     litVal;
} astNode;


#define SpaceCase case '\n': case '\t': case ' '
#define typeCharCase \
case numType:\
case narType:\
case parType:\
case thruType

BufType(char,     charBuf);
BufType(astNode,  astNodeBuf);
BufType(astNode*, astNodePBuf);

astNodeBuf   astNodes;
astNodePBuf  astKids;


void deorphan(astNode *const orphan) {
	for (astNode *p = orphan-1; p >= astNodes.data; p--) {
		if (p->def.fn == sl_root && p->kidCount) {
			printf("ERROR in line %i:\n\t'", orphan->line);
			printUpTo(orphan->def.name, tokSep);
			printf("' has no parent.\n");
			return;
		}
		if (p->kidCount < p->def.arity) {
			astKids.data[p->kidsIndx + p->kidCount] = orphan;
			p->kidCount++;
			return;
		}
	}
	_ShouldNotBeHere_;
}

void nodeFromToken(char *token, uint32_t line) {
	printf("nodeFromToken: "); printUpTo(token, tokSep); puts("");
	pushEmpty_astNodeBuf(&astNodes);
	astNode *const lastNode = plast_astNodeBuf(astNodes);
	// numlit
	if (
		('0' <= *token && *token <= '9')  ||  
		(*token == '-' && '0' <= token[1] && token[1] <= '9')
	) {
		lastNode->def.name   = token;
		lastNode->def.arity  = 0;
		lastNode->def.fn     = sl_numLit;
		lastNode->kidsIndx   = UINT32_MAX;
		lastNode->kidCount   = 0;
		lastNode->line       = line;
		lastNode->litVal.num = atof(token);
		//printf("numLit: %f\n", lastNode->litVal.num);
		deorphan(lastNode);
		return;
	}
	// stdlib
	fr (i, cscdStdLibCount) {
		if (matchUpTo(tokenPastType(&cscdStdLib[i].name[0]), token, tokSep)) {
			lastNode->def        = cscdStdLib[i];
			lastNode->line       = line;
			lastNode->kidCount   = 0;
			lastNode->litVal.num = 0;
			const uint32_t arity = lastNode->def.arity; 
			if (arity) {
				pushNEmpty_astNodePBuf(&astKids, arity);
				lastNode->kidsIndx = astKids.count - arity; 
			}
			deorphan(lastNode);
			return;
		}
	}
	// not found
	_ShouldNotBeHere_;
}
void rootFromToken(char *token, uint32_t line) {
	printf("rootFromToken: "); printUpTo(token, tokSep); puts("");
	pushEmpty_astNodeBuf(&astNodes);
	astNode *const lastNode = plast_astNodeBuf(astNodes);
	lastNode->def.name   = token;
	lastNode->def.arity  = 1;
	lastNode->def.fn     = sl_root;
	lastNode->line       = line;
	lastNode->kidCount   = 0;
	lastNode->litVal.num = 0;
	for (astNode *n = lastNode; n->def.fn != sl_root; n--) {
		if (n < astNodes.data) {
			_ShouldNotBeHere_;
			return;
		}
		if (n->kidCount < n->def.arity) {
			printf("ERROR in line %i:\n\t''", line);
			printUpTo(n->def.name, tokSep);
			printf("' has too few arguments.\n");
			return;
		}
	}
	pushEmpty_astNodePBuf(&astKids);
	lastNode->kidsIndx = astKids.count-1; 
}

void handleToken(char *token, uint32_t line) {
	switch (*token) {typeCharCase:
		rootFromToken(token, line);
		return;
	}
	nodeFromToken(token, line);
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
	uint32_t curLine      = 1;
	uint32_t commentDepth = 0;
	astNodes       = init_astNodeBuf(32);
	astKids        = init_astNodePBuf(32);
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
				handleToken(tokenStart, inchar == '\n' ? curLine-1 : curLine);
				tokenStart = &tokens.data[tokens.count];
				continue;
		}
		push_charBuf(&tokens, inchar);
	}
	if (commentDepth) puts("Warning: unclosed comment\n");
	fr (i, astNodes.count) {
		printf("%3i: ", astNodes.data[i].line);
		printUpTo(astNodes.data[i].def.name, tokSep);
		puts("");
	}
	fclose(infile);
	free(astNodes.data);
	free(astKids.data);
	free(tokens.data);
	return 0;
}
