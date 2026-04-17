/*
Authored by: Preston Vardaman
Iris Compiler Project
file type: .ir
*/

#include "compiler.h"

#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 4 || std::string(argv[1]) != "-o") {
        std::cerr << "Usage: iris -o <output> <file.ir>\n";
        return 1;
    }

    std::ifstream file(argv[3]);
    if (!file) {
        std::cerr << "Cannot open file: " << argv[3] << "\n";
        return 1;
    }
    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    return compile(source, argv[2]) ? 0 : 1;
}
