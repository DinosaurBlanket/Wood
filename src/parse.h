#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "buf.h"

#define _ShouldNotBeHere_ printf("Should Not Be Here: line %i of %s\n", __LINE__, __FILE__)
#define fr(i, bound) for (int i = 0; i < (bound); i++)

#define singlTypeChar ':'
#define arrayTypeChar '|'
#define doubleBufChar '\\'
#define cmntStart '('
#define cmntEnd   ')'
#define tokSep    ' ' // token separator

typedef union {
	double num;
	double _unusedfornow[4];
} typeBox;

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

BufTypeHead(char,    charBuf);
BufTypeHead(astNode, astNodeBuf);

void parse(FILE *infile, charBuf *tokens, astNodeBuf *astNodes);
void printUpTo(const char *toPrint, char upTo);
