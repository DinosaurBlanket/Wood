#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "buf.h"

#define singlTypeChar ':'
#define arrayTypeChar '|'
#define doubleBufChar '\\'
#define cmntStart '('
#define cmntEnd   ')'
#define tokSep    ' ' // token separator

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
void printErrorHead(uint32_t line) {printf("ERROR in line %i:\n\t", line);}


typedef union {
	double num;
	double _unusedfornow[4];
} typeBox;

enum astNodeIds {
  // don't return anything
    // not outputs
      ani_structDef,
      ani_fnDef,
      ani_rootDef,
      ani_atfile,
    // outputs
      ani_text,
      ani_video,
      ani_audio,
      ani_baxit,
      ani_proque,
      ani_prospawn,
  // return something
    ani_builtinCall,
    ani_fnCall,
    ani_paramCall,
    ani_graft,
    ani_graftCall,
    ani_structFunnel,
    ani_rootCall,
    ani_numLit,
    ani_strLit,
    ani_arrLitBeg,
    ani_arrLitEnd
};

typedef struct {
	char *name;    // includes types
  int   kidsReq; // number of nodes that should link to this as "parent"
} astNodeDef;

#define builtinsCount 5
const astNodeDef builtins[builtinsCount] = {
	{.name=" :num + :num l :num r "+6, .kidsReq=2},
	{.name=" :num - :num l :num r "+6, .kidsReq=2},
	{.name=" :num * :num l :num r "+6, .kidsReq=2},
	{.name=" :num / :num l :num r "+6, .kidsReq=2},
	{.name=" :num nine "+6,            .kidsReq=0}
};

typedef struct {
	char           *name;
  int             kidsReq;
	int             kidCount; // incremented until it matches parent's kidsReq
	int             id;
	struct astNode *parent;
  struct astNode *idSource; // source of what's being called (fnDef, root, structDef, multiplexer)
  int             idSourceRel; // index of thing relative to source (nth param, struct member, multiplexer ctx)
	typeBox         litVal;
	int             line;
} astNode;

#define SpaceCase case '\n': case '\t': case ' '
#define typeCharCase case singlTypeChar: case arrayTypeChar: case doubleBufChar


BufType(char,     charBuf);
BufType(astNode,  astNodeBuf);

astNodeBuf   astNodes;

void deorphan(astNode *const orphan) {
	for (astNode *p = orphan-1; p >= astNodes.data; p--) {
		if (p->id == ani_rootDef && p->kidCount) {
			printErrorHead(orphan->line);
			printf("'"); printUpTo(orphan->name, tokSep);
			printf("' has no parent.\n");
			return;
		}
		if (p->kidCount < p->kidsReq) {
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
		lastNode->name        = token;
		lastNode->kidsReq     = 0;
		lastNode->id          = ani_numLit;
		lastNode->parent      = NULL;
		lastNode->idSource    = NULL;
		lastNode->idSourceRel = 0;
		lastNode->litVal.num  = atof(token);
		lastNode->line        = line;
		//printf("numLit: %f\n", lastNode->litVal.num);
		deorphan(lastNode);
		return;
	}
	// builtin
	fr (i, builtinsCount) {
		if (matchUpTo(builtins[i].name, token, tokSep)) {
			lastNode->name        = builtins[i].name;
			lastNode->kidsReq     = builtins[i].kidsReq;
			lastNode->kidCount    = 0;
			lastNode->id          = ani_builtinCall;
			lastNode->parent      = NULL;
			lastNode->idSource    = NULL;
			lastNode->idSourceRel = 0;
			lastNode->litVal.num  = 0;
			lastNode->line        = line;
			deorphan(lastNode);
			return;
		}
	}
	// local
	fr (i, astNodes.count) {
		// root
		if (
			astNodes.data[i].id == ani_rootDef  &&
			matchUpTo(astNodes.data[i].name, token, tokSep)
		) {
			lastNode->name        = astNodes.data[i].name;
			lastNode->kidsReq     = 0;
			lastNode->kidCount    = 0;
			lastNode->id          = ani_rootCall;
			lastNode->parent      = NULL;
			lastNode->idSource    = NULL;
			lastNode->idSourceRel = 0;
			lastNode->litVal.num  = 0;
			lastNode->line        = line;
			deorphan(lastNode);
			return;
		}
	}
	// not found
	printErrorHead(line);
	printf("'"); printUpTo(token, tokSep);
	printf("' was not recognized.\n");
}
void rootFromToken(char *token, uint32_t line) {
	printf("rootFromToken: "); printUpTo(token, tokSep); puts("");
	pushEmpty_astNodeBuf(&astNodes);
	astNode *const lastNode = plast_astNodeBuf(astNodes);
	lastNode->name        = token;
	lastNode->kidsReq     = 1;
	lastNode->kidCount    = 0;
	lastNode->id          = ani_rootDef;
	lastNode->parent      = NULL;
	lastNode->idSource    = NULL;
	lastNode->idSourceRel = 0;
	lastNode->litVal.num  = 0;
	lastNode->line        = line;
	for (astNode *n = lastNode; n->id != ani_rootDef; n--) {
		if (n < astNodes.data) {
			_ShouldNotBeHere_;
			return;
		}
		if (n->kidCount < n->kidsReq) {
			printErrorHead(line);
			printf("'"); printUpTo(n->name, tokSep);
			printf("' has too few arguments.\n");
			return;
		}
	}
}

void handleToken(char *token, uint32_t line) {
	static int nextToken = -1;
	switch (*token) {typeCharCase:
		nextToken = ani_rootDef;
		return;
	}
	if (nextToken == ani_rootDef) {
		nextToken = -1;
		rootFromToken(token, line);
		return;
	}
	nodeFromToken(token, line);
}


int main(int argc, char **argv) {
	if (argc < 2) {
		puts("usage:\nwood HelloWorld.wood\n");
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
		printUpTo(astNodes.data[i].name, tokSep);
		puts("");
	}
	fclose(infile);
	free(astNodes.data);
	free(tokens.data);
	return 0;
}
