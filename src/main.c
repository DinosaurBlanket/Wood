#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "buf.h"

#define singlTypeChar ':'
#define arrayTypeChar '|'
#define doubleBufChar '\\'
#define cmntStart '('
#define cmntEnd   ')'
#define tokSep    '\n' // token separator
//#define tokSepStr "\n"

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
      ani_root,
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
	char *name;         // includes types
  int   kidsRequired; // number of nodes that should link to this as "parent"
} astNodeDef;

#define builtinsCount 5
const astNodeDef builtins[builtinsCount] = {
	{.name=":num + :num l :num r", .kidsRequired=2},
	{.name=":num - :num l :num r", .kidsRequired=2},
	{.name=":num * :num l :num r", .kidsRequired=2},
	{.name=":num / :num l :num r", .kidsRequired=2},
	{.name=":num nine",            .kidsRequired=0}
};

typedef struct {
  astNodeDef      def;
	int             id;
  int             kidCount;  // incremented until it matches parent's kidsRequired
	struct astNode *parent;
  struct astNode *idSource;  // source of what's being called (fnDef, root, structDef, multiplexer)
  int             idSourceRelation; // index of thing relative to source (nth param, struct member, multiplexer ctx)
} astNode;

#define SpaceCase case '\n': case '\t': case ' '
#define typeCharCase case singlTypeChar: case arrayTypeChar: case doubleBufChar


BufType(char,     charBuf);
BufType(astNode,  astNodeBuf);

astNodeBuf   astNodes;










void deorphan(astNode *const orphan) {
	for (astNode *p = orphan-1; p >= astNodes.data; p--) {
		if (p->def.fn == sl_rootDef && p->kidCount) {
			printErrorHead(orphan->line);
			printf("'"); printUpTo(orphan->def.name, tokSep);
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
	fr (i, woodStdLibCount) {
		if (matchUpTo(tokenPastType(&woodStdLib[i].name[0]), token, tokSep)) {
			lastNode->def        = woodStdLib[i];
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
	// local
	fr (i, astNodes.count) {
		// root
		if (
			astNodes.data[i].def.fn == sl_rootDef  &&
			matchUpTo(tokenPastType(astNodes.data[i].def.name), token, tokSep)
		) {
			lastNode->def.name   = astNodes.data[i].def.name;
			lastNode->def.arity  = 0;
			lastNode->def.fn     = sl_rootRef;
			lastNode->line       = line;
			lastNode->kidsIndx   = UINT32_MAX;
			lastNode->kidCount   = 0;
			lastNode->litVal.num = 0;
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
	lastNode->def.name   = token;
	lastNode->def.arity  = 1;
	lastNode->def.fn     = sl_rootDef;
	lastNode->line       = line;
	lastNode->kidCount   = 0;
	lastNode->litVal.num = 0;
	for (astNode *n = lastNode; n->def.fn != sl_rootDef; n--) {
		if (n < astNodes.data) {
			_ShouldNotBeHere_;
			return;
		}
		if (n->kidCount < n->def.arity) {
			printErrorHead(line);
			printf("'"); printUpTo(n->def.name, tokSep);
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
