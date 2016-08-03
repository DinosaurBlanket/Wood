#include <fstream>
#include <string>
#include <iostream>

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
