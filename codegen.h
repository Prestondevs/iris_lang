#pragma once

#include "ast.h"
#include <ostream>
#include <vector>
#include <string>
#include <unordered_map>

class AsmCodeGen {
public:
    explicit AsmCodeGen(std::ostream& out);
    void generate(Program* prog);

private:
    std::ostream& out;

    std::vector<std::string> floatLiterals;
    std::vector<std::string> stringLiterals;
    std::unordered_map<std::string, int> floatLitIdx;
    std::unordered_map<std::string, int> strLitIdx;

    std::unordered_map<std::string, int> locals;
    int localCount;
    int tempDepth;
    bool hasIrisMain;

    static int slotOffset(int k) { return -16 * (k + 1); }
    int tempSlotOffset(int d) const { return slotOffset(localCount + d); }

    static std::string fmtOff(int off);
    static std::string nasmStrLit(const std::string& s);

    int getFloatIdx(double v);
    int getStrIdx(const std::string& s);

    void collectLiterals(ASTNode* node);
    void buildLocalsMap(FunDeclStmt* f);

    void emitExpr(ASTNode* node);
    void emitStmt(ASTNode* node);
    void emitFunHeader(const std::string& label, int frameSize);
    void emitFun(FunDeclStmt* f);
    void emitCMain(Program* prog);
};
