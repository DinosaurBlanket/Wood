#include <stdio.h>
#include <stdint.h>
#include "buf.h"

const char numType = '.';
const char narType = ':';
const char parType = '|';
const char anyType = '&';

bufType(char, charBuf)

typedef union {
	double num;
	//f32buf narray;
	charBuf  parray;
} anyBox;

typedef struct {
	char  *name;  // includes parameters, doesn't include "fn "
	void  *fn;    // will hold function pointers of many types, or NULL
	anyBox value; // for nodes that output constant, used when fn == NULL
} astNodeDef;

double sl_add(double a, double b) {return a + b;}
//const astNodeDef and_add = {".+ .a .b", sl_add, 0};
double sl_sub(double a, double b) {return a + b;}
double sl_div(double a, double b) {return a + b;}
double sl_mul(double a, double b) {return a + b;}

#define cscdStdLibCount 4
const astNodeDef cscdStdLib[cscdStdLibCount] = {
	{.name = ".+ .a .b", .fn = sl_add, .value = 0},
	{.name = ".- .a .b", .fn = sl_sub, .value = 0},
	{.name = "./ .a .b", .fn = sl_div, .value = 0},
	{.name = ".* .a .b", .fn = sl_mul, .value = 0}
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


void handleToken(charBuf *t) {
	if (!t->count) return;
	puts(t->data);
	clear_charBuf(t);
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
	
	uint32_t curLine = 0;
	uint32_t commentDepth = 0;
	charBuf token = init_charBuf(128);
	for (
		char inchar = fgetc(infile);
		inchar != EOF;
		inchar = fgetc(infile)
	) {
		if (commentDepth) {
			switch(inchar) {
				case '(': commentDepth++; break;
				case ')': commentDepth--; break;
			}
			continue;
		}
		switch(inchar) {
			case '\n': curLine++; // fall
			case '\t': // fall
			case ' ' :
				handleToken(&token);
				break;
			case '(' :
				handleToken(&token);
				commentDepth = 1;
				break;
			default: push_charBuf(&token, inchar);
		}
	}
	if (commentDepth) puts("Warning: unclosed comment\n");
	fclose(infile);
	
	return 0;
}
