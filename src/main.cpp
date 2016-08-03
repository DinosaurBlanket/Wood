#include <fstream>
#include <string>
#include <iostream>

void handleToken(std::string &t) {
	std::cout << t << std::endl;
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
	
	uint32_t curLine;
	std::string token;
	char curFChar = '\0';
	char prvFChar = '\0';
	while (infile.peek() != EOF) {
		prvFChar = curFChar;
		curFChar = infile.get();
		switch(curFChar) {
			case '\n': curLine++; // fall
			case '\t': // fall
			case ' ' : // fall
				switch (prvFChar) {
					case '\n': break;
					case '\t': break;
					case ' ' : break;
					default:
						handleToken(token);
						token.clear();
				}
				break;
			default:
				token.push_back(curFChar);
		}
	}
	
	infile.close();
	return 0;
}
