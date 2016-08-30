
#include "parse.h"


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
	charBuf    tokens   = init_charBuf(256);
	astNodeBuf astNodes = init_astNodeBuf(32);
	parse(infile, &tokens, &astNodes);
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
