#include <fstream>
#include <string>
#include <iostream>


int main(int argc, char** argv ) {
	if (argc < 2) {
		puts("usage:\ncscd HelloWorld.cscd");
		return 1;
	}
  char* fileName = argv[1];
	std::ifstream infile(fileName);
	if (infile.fail()) {
		printf("could not open '%s'\n", fileName);
		return 2;
	}
	
	std::string s;
	while (infile.peek() != EOF) s.push_back(infile.get());
	std::cout << s;
	
	infile.close();
	return 0;
}
