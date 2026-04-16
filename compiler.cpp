#ifdef _WIN32
    #define popen _popen
    #define pclose _pclose
#endif

#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>

bool compile(const std::string& source, const std::string& outputName) {
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    auto program = parser.parse();

    std::ostringstream asmSrc;
    AsmCodeGen codegen(asmSrc);
    codegen.generate(program.get());

    std::string tmpObj = outputName + ".iris.o";
    {
        std::string asCmd = "as --64 -o \"" + tmpObj + "\" -";
        FILE* proc = popen(asCmd.c_str(), "w");
        if (!proc) {
            std::cerr << "Cannot run assembler\n";
            return false;
        }
        const std::string& asmStr = asmSrc.str();
        fwrite(asmStr.c_str(), 1, asmStr.size(), proc);
        if (pclose(proc) != 0) {
            std::cerr << "Assembly failed\n";
            return false;
        }
    }

    std::string cmd = "gcc -o \"" + outputName + "\" \"" + tmpObj + "\" -no-pie";
    int ret = std::system(cmd.c_str());
    std::remove(tmpObj.c_str());

    if (ret != 0) {
        std::cerr << "Link failed\n";
        return false;
    }

    return true;
}
