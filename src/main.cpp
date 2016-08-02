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
	if (!infile) {
		printf("could not open '%s'\n", fileName);
		return 2;
	}
	
	std::string s;
	char fchar;
	for(;;) {
		fchar = infile.get();
		if (fchar == EOF) break;
		s.push_back(fchar);
	}
	std::cout << s;
	
	infile.close();
	return 0;
}
