#include <vector>
#include <string>
#include <fstream>
#include <iostream>

const char numType = '.';
const char narType = ':';
const char parType = '|';
const char anyType = '&';


union cscdTypeBox  {
	double               num;
	std::vector<float>   narray;
	std::vector<uint8_t> parray;
};


struct astNodeDef {
	std::string  name;  // includes parameters, doesn't include "fn "
	void        *fn;    // will hold function pointers of many types, or NULL
	cscdTypeBox  value; // for nodes that output constant, used when fn == NULL
};

struct astNode {
	std::string          name; // same as astNodeDef
	std::vector<astNode> kids;
};

struct astRoot {
	std::string name;
	astNode     init;
	astNode     reev;
};


void handleToken(std::string &t) {
	if (!t.length()) return;
	std::cout << t << std::endl;
	t.clear();
}

int main(int argc, char** argv ) {
	if (argc < 2) {
		std::cout << "usage:\ncscd HelloWorld.cscd\n";
		return 1;
	}
  std::string fileName = argv[1];
	std::ifstream infile(fileName);
	if (infile.fail()) {
		std::cout << "could not open '" << fileName << "'\n";
		return 2;
	}
	
	uint32_t curLine = 0;
	uint32_t commentDepth = 0;
	char inchar = '\0';
	std::string token;
	while (infile.peek() != EOF) {
		inchar = infile.get();
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
				handleToken(token);
				break;
			case '(' :
				handleToken(token);
				commentDepth = 1;
				break;
			default:
				token.push_back(inchar);
		}
	}
	if (commentDepth) std::cout << "Warning: unclosed comment\n";
	infile.close();
	
	return 0;
}
