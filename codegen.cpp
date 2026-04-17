#include "codegen.h"

#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <iomanip>

static const char* GAS_RUNTIME = R"(
iris_rout:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    cmp rdi, 0
    je .Lrout_int
    cmp rdi, 1
    je .Lrout_float
    lea rdi, [rip + _fmt_str]
    xor eax, eax
    call printf
    jmp .Lrout_done
.Lrout_int:
    lea rdi, [rip + _fmt_int]
    xor eax, eax
    call printf
    jmp .Lrout_done
.Lrout_float:
    movq xmm0, rsi
    lea rdi, [rip + _fmt_flt]
    mov eax, 1
    call printf
.Lrout_done:
    xor eax, eax
    xor edx, edx
    leave
    ret

iris_rin:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    lea rdi, [rip + _rin_buf]
    mov rsi, 4096
    mov rdx, [rip + stdin]
    call fgets
    test rax, rax
    jz .Lrin_empty
    lea rdi, [rip + _rin_buf]
    call strlen
    test rax, rax
    jz .Lrin_done
    lea rdi, [rip + _rin_buf]
    cmp BYTE PTR [rdi + rax - 1], 10
    jne .Lrin_done
    mov BYTE PTR [rdi + rax - 1], 0
    jmp .Lrin_done
.Lrin_empty:
    lea rdi, [rip + _rin_buf]
    mov BYTE PTR [rdi], 0
.Lrin_done:
    mov eax, 2
    lea rdx, [rip + _rin_buf]
    leave
    ret

iris_rt_add:
    push rbp
    mov rbp, rsp
    cmp rdi, 2
    jne .Ladd_not_str
    cmp rdx, 2
    jne .Ladd_not_str
    mov r14, rsi
    mov r15, rcx
    mov rdi, rsi
    call strlen
    mov r12, rax
    mov rdi, r15
    call strlen
    lea rdi, [r12 + rax + 1]
    call malloc
    mov rdi, rax
    mov rsi, r14
    mov r13, rax
    call strcpy
    mov rdi, r13
    mov rsi, r15
    call strcat
    mov eax, 2
    mov rdx, r13
    pop rbp
    ret
.Ladd_not_str:
    cmp rdi, 1
    je .Ladd_float
    cmp rdx, 1
    je .Ladd_float
    add rsi, rcx
    xor eax, eax
    mov rdx, rsi
    pop rbp
    ret
.Ladd_float:
    cmp rdi, 1
    jne .Ladd_d1_int
    movq xmm0, rsi
    jmp .Ladd_d2
.Ladd_d1_int:
    cvtsi2sd xmm0, rsi
.Ladd_d2:
    cmp rdx, 1
    jne .Ladd_d2_int
    movq xmm1, rcx
    jmp .Ladd_do
.Ladd_d2_int:
    cvtsi2sd xmm1, rcx
.Ladd_do:
    addsd xmm0, xmm1
    movq rdx, xmm0
    mov eax, 1
    pop rbp
    ret

iris_rt_sub:
    push rbp
    mov rbp, rsp
    cmp rdi, 1
    je .Lsub_float
    cmp rdx, 1
    je .Lsub_float
    sub rsi, rcx
    xor eax, eax
    mov rdx, rsi
    pop rbp
    ret
.Lsub_float:
    cmp rdi, 1
    jne .Lsub_d1_int
    movq xmm0, rsi
    jmp .Lsub_d2
.Lsub_d1_int:
    cvtsi2sd xmm0, rsi
.Lsub_d2:
    cmp rdx, 1
    jne .Lsub_d2_int
    movq xmm1, rcx
    jmp .Lsub_do
.Lsub_d2_int:
    cvtsi2sd xmm1, rcx
.Lsub_do:
    subsd xmm0, xmm1
    movq rdx, xmm0
    mov eax, 1
    pop rbp
    ret

iris_rt_mul:
    push rbp
    mov rbp, rsp
    cmp rdi, 1
    je .Lmul_float
    cmp rdx, 1
    je .Lmul_float
    imul rsi, rcx
    xor eax, eax
    mov rdx, rsi
    pop rbp
    ret
.Lmul_float:
    cmp rdi, 1
    jne .Lmul_d1_int
    movq xmm0, rsi
    jmp .Lmul_d2
.Lmul_d1_int:
    cvtsi2sd xmm0, rsi
.Lmul_d2:
    cmp rdx, 1
    jne .Lmul_d2_int
    movq xmm1, rcx
    jmp .Lmul_do
.Lmul_d2_int:
    cvtsi2sd xmm1, rcx
.Lmul_do:
    mulsd xmm0, xmm1
    movq rdx, xmm0
    mov eax, 1
    pop rbp
    ret

iris_rt_div:
    push rbp
    mov rbp, rsp
    cmp rdi, 1
    jne .Ldiv_d1_int
    movq xmm0, rsi
    jmp .Ldiv_d2
.Ldiv_d1_int:
    cvtsi2sd xmm0, rsi
.Ldiv_d2:
    cmp rdx, 1
    jne .Ldiv_d2_int
    movq xmm1, rcx
    jmp .Ldiv_do
.Ldiv_d2_int:
    cvtsi2sd xmm1, rcx
.Ldiv_do:
    divsd xmm0, xmm1
    movq rdx, xmm0
    mov eax, 1
    pop rbp
    ret

iris_rt_idiv:
    push rbp
    mov rbp, rsp
    cmp rdi, 1
    jne .Lidiv_d1_int
    movq xmm0, rsi
    cvttsd2si rax, xmm0
    jmp .Lidiv_d2
.Lidiv_d1_int:
    mov rax, rsi
.Lidiv_d2:
    cmp rdx, 1
    jne .Lidiv_d2_int
    movq xmm0, rcx
    cvttsd2si rcx, xmm0
.Lidiv_d2_int:
    cqo
    idiv rcx
    mov rdx, rax
    xor eax, eax
    pop rbp
    ret
)";

AsmCodeGen::AsmCodeGen(std::ostream& out)
    : out(out), localCount(0), tempDepth(0), hasIrisMain(false) {}

std::string AsmCodeGen::fmtOff(int off) {
    if (off < 0) return " - " + std::to_string(-off);
    if (off > 0) return " + " + std::to_string(off);
    return "";
}

std::string AsmCodeGen::nasmStrLit(const std::string& s) {
    if (s.empty()) return "0";
    std::string r;
    bool inQ = false;
    for (unsigned char c : s) {
        if (c >= 32 && c != 127 && c != '"' && c != '\\') {
            if (!inQ) { if (!r.empty()) r += ", "; r += '"'; inQ = true; }
            r += (char)c;
        } else {
            if (inQ) { r += '"'; inQ = false; }
            if (!r.empty()) r += ", ";
            r += std::to_string((int)c);
        }
    }
    if (inQ) r += '"';
    return r;
}

int AsmCodeGen::getFloatIdx(double v) {
    uint64_t bits;
    std::memcpy(&bits, &v, 8);
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << bits;
    std::string key = oss.str();
    auto it = floatLitIdx.find(key);
    if (it != floatLitIdx.end()) return it->second;
    int idx = (int)floatLiterals.size();
    floatLiterals.push_back(key);
    floatLitIdx[key] = idx;
    return idx;
}

int AsmCodeGen::getStrIdx(const std::string& s) {
    auto it = strLitIdx.find(s);
    if (it != strLitIdx.end()) return it->second;
    int idx = (int)stringLiterals.size();
    stringLiterals.push_back(s);
    strLitIdx[s] = idx;
    return idx;
}

void AsmCodeGen::collectLiterals(ASTNode* node) {
    if (!node) return;
    if (auto* n = dynamic_cast<FloatLitExpr*>(node))       { getFloatIdx(n->value); }
    else if (auto* n = dynamic_cast<StringLitExpr*>(node)) { getStrIdx(n->value); }
    else if (auto* n = dynamic_cast<BinaryExpr*>(node))    { collectLiterals(n->left.get()); collectLiterals(n->right.get()); }
    else if (auto* n = dynamic_cast<CallExpr*>(node))      { for (auto& a : n->args) collectLiterals(a.get()); }
    else if (auto* n = dynamic_cast<VarDeclStmt*>(node))   { collectLiterals(n->initializer.get()); }
    else if (auto* n = dynamic_cast<ReturnStmt*>(node))    { collectLiterals(n->value.get()); }
    else if (auto* n = dynamic_cast<ExprStmt*>(node))      { collectLiterals(n->expr.get()); }
    else if (auto* n = dynamic_cast<BlockStmt*>(node))     { for (auto& s : n->stmts) collectLiterals(s.get()); }
    else if (auto* n = dynamic_cast<FunDeclStmt*>(node))   { collectLiterals(n->body.get()); }
    else if (auto* n = dynamic_cast<Program*>(node))       { for (auto& d : n->declarations) collectLiterals(d.get()); }
}

void AsmCodeGen::buildLocalsMap(FunDeclStmt* f) {
    locals.clear();
    localCount = 0;
    tempDepth  = 0;
    for (size_t i = 0; i < f->params.size(); i++) {
        locals[f->params[i]] = slotOffset((int)i);
        localCount++;
    }
    if (auto* block = dynamic_cast<BlockStmt*>(f->body.get())) {
        for (auto& s : block->stmts) {
            if (auto* v = dynamic_cast<VarDeclStmt*>(s.get())) {
                if (locals.find(v->name) == locals.end()) {
                    locals[v->name] = slotOffset(localCount++);
                }
            }
        }
    }
}

void AsmCodeGen::emitFunHeader(const std::string& label, int frameSize) {
    out << label << ":\n";
    out << "    push rbp\n";
    out << "    mov rbp, rsp\n";
    out << "    sub rsp, " << frameSize << "\n";
}

void AsmCodeGen::emitFun(FunDeclStmt* f) {
    buildLocalsMap(f);
    int frameSize = (localCount + 8) * 16;

    std::string label = (f->name == "main") ? "main" : "_iris_" + f->name;
    emitFunHeader(label, frameSize);

    static const char* tRegs[] = {"rdi", "rdx", "r8"};
    static const char* dRegs[] = {"rsi", "rcx", "r9"};
    int nP = (int)std::min(f->params.size(), (size_t)3);
    for (int i = 0; i < nP; i++) {
        int off = slotOffset(i);
        out << "    mov [rbp" << fmtOff(off)   << "], " << tRegs[i] << "\n";
        out << "    mov [rbp" << fmtOff(off+8) << "], " << dRegs[i] << "\n";
    }

    if (auto* block = dynamic_cast<BlockStmt*>(f->body.get()))
        for (auto& s : block->stmts) emitStmt(s.get());

    out << "    xor eax, eax\n";
    out << "    xor edx, edx\n";
    out << "    leave\n";
    out << "    ret\n\n";
}

void AsmCodeGen::emitCMain(Program* prog) {
    locals.clear();
    localCount = 0;
    tempDepth  = 0;

    for (auto& d : prog->declarations) {
        if (!dynamic_cast<FunDeclStmt*>(d.get())) {
            if (auto* v = dynamic_cast<VarDeclStmt*>(d.get())) {
                if (locals.find(v->name) == locals.end())
                    locals[v->name] = slotOffset(localCount++);
            }
        }
    }

    int frameSize = (localCount + 8) * 16;
    emitFunHeader("main", frameSize);

    for (auto& d : prog->declarations)
        if (!dynamic_cast<FunDeclStmt*>(d.get()))
            emitStmt(d.get());

    out << "    xor eax, eax\n";
    out << "    leave\n";
    out << "    ret\n\n";
}

void AsmCodeGen::emitExpr(ASTNode* node) {
    if (auto* n = dynamic_cast<IntLitExpr*>(node)) {
        out << "    mov rax, 0\n";
        out << "    mov rdx, " << n->value << "\n";

    } else if (auto* n = dynamic_cast<FloatLitExpr*>(node)) {
        int idx = getFloatIdx(n->value);
        out << "    mov rax, 1\n";
        out << "    mov rdx, [rip + _flt" << idx << "]\n";

    } else if (auto* n = dynamic_cast<StringLitExpr*>(node)) {
        int idx = getStrIdx(n->value);
        out << "    mov rax, 2\n";
        out << "    lea rdx, [rip + _str" << idx << "]\n";

    } else if (auto* n = dynamic_cast<IdentExpr*>(node)) {
        auto it = locals.find(n->name);
        if (it == locals.end()) {
            std::cerr << "Undefined variable: " << n->name << "\n";
            std::exit(1);
        }
        int off = it->second;
        out << "    mov rax, [rbp" << fmtOff(off)   << "]\n";
        out << "    mov rdx, [rbp" << fmtOff(off+8) << "]\n";

    } else if (auto* n = dynamic_cast<BinaryExpr*>(node)) {
        if (tempDepth >= 8) {
            std::cerr << "Expression nesting too deep\n"; std::exit(1);
        }
        emitExpr(n->left.get());
        int toff = tempSlotOffset(tempDepth);
        out << "    mov [rbp" << fmtOff(toff)   << "], rax\n";
        out << "    mov [rbp" << fmtOff(toff+8) << "], rdx\n";
        tempDepth++;
        emitExpr(n->right.get());
        tempDepth--;
        out << "    mov rcx, rdx\n";
        out << "    mov rdx, rax\n";
        out << "    mov rdi, [rbp" << fmtOff(toff)   << "]\n";
        out << "    mov rsi, [rbp" << fmtOff(toff+8) << "]\n";
        const char* fn =
            n->op == "+"  ? "iris_rt_add"  :
            n->op == "-"  ? "iris_rt_sub"  :
            n->op == "*"  ? "iris_rt_mul"  :
            n->op == "/"  ? "iris_rt_div"  :
            n->op == "//" ? "iris_rt_idiv" : "iris_rt_add";
        out << "    call " << fn << "\n";

    } else if (auto* n = dynamic_cast<CallExpr*>(node)) {
        if (n->callee == "rout") {
            if (!n->args.empty()) {
                emitExpr(n->args[0].get());
                out << "    mov rdi, rax\n";
                out << "    mov rsi, rdx\n";
            } else {
                out << "    xor edi, edi\n";
                out << "    xor esi, esi\n";
            }
            out << "    call iris_rout\n";

        } else if (n->callee == "rin") {
            out << "    call iris_rin\n";

        } else {
            int savedDepth = tempDepth;
            int argCount   = (int)std::min(n->args.size(), (size_t)3);
            for (int i = 0; i < argCount; i++) {
                emitExpr(n->args[i].get());
                int toff = tempSlotOffset(savedDepth + i);
                out << "    mov [rbp" << fmtOff(toff)   << "], rax\n";
                out << "    mov [rbp" << fmtOff(toff+8) << "], rdx\n";
                tempDepth = savedDepth + i + 1;
            }
            tempDepth = savedDepth;
            if (argCount >= 1) {
                int toff = tempSlotOffset(savedDepth);
                out << "    mov rdi, [rbp" << fmtOff(toff)   << "]\n";
                out << "    mov rsi, [rbp" << fmtOff(toff+8) << "]\n";
            }
            if (argCount >= 2) {
                int toff = tempSlotOffset(savedDepth + 1);
                out << "    mov rdx, [rbp" << fmtOff(toff)   << "]\n";
                out << "    mov rcx, [rbp" << fmtOff(toff+8) << "]\n";
            }
            if (argCount >= 3) {
                int toff = tempSlotOffset(savedDepth + 2);
                out << "    mov r8,  [rbp" << fmtOff(toff)   << "]\n";
                out << "    mov r9,  [rbp" << fmtOff(toff+8) << "]\n";
            }
            out << "    call _iris_" << n->callee << "\n";
        }
    }
}

void AsmCodeGen::emitStmt(ASTNode* node) {
    if (auto* n = dynamic_cast<VarDeclStmt*>(node)) {
        emitExpr(n->initializer.get());
        auto it = locals.find(n->name);
        if (it == locals.end()) {
            std::cerr << "Internal error: undeclared variable " << n->name << "\n";
            std::exit(1);
        }
        int off = it->second;
        out << "    mov [rbp" << fmtOff(off)   << "], rax\n";
        out << "    mov [rbp" << fmtOff(off+8) << "], rdx\n";

    } else if (auto* n = dynamic_cast<ReturnStmt*>(node)) {
        if (n->value) emitExpr(n->value.get());
        else { out << "    xor eax, eax\n"; out << "    xor edx, edx\n"; }
        out << "    leave\n";
        out << "    ret\n";

    } else if (auto* n = dynamic_cast<ExprStmt*>(node)) {
        emitExpr(n->expr.get());

    } else if (auto* n = dynamic_cast<BlockStmt*>(node)) {
        for (auto& s : n->stmts) emitStmt(s.get());
    }
}

void AsmCodeGen::generate(Program* prog) {
    for (auto& d : prog->declarations)
        if (auto* f = dynamic_cast<FunDeclStmt*>(d.get()))
            if (f->name == "main") { hasIrisMain = true; break; }

    collectLiterals(prog);

    out << ".intel_syntax noprefix\n\n";

    out << ".section .data\n";
    out << "_fmt_int: .byte 37,108,100,10,0\n";
    out << "_fmt_flt: .byte 37,103,10,0\n";
    out << "_fmt_str: .byte 37,115,10,0\n";
    for (size_t i = 0; i < stringLiterals.size(); i++) {
        out << "_str" << i << ": ";
        const std::string& s = stringLiterals[i];
        if (s.empty()) {
            out << ".byte 0\n";
        } else {
            out << ".byte ";
            for (size_t j = 0; j < s.size(); j++) {
                if (j) out << ",";
                out << (int)(unsigned char)s[j];
            }
            out << ",0\n";
        }
    }
    for (size_t i = 0; i < floatLiterals.size(); i++)
        out << "_flt" << i << ": .quad " << floatLiterals[i] << "\n";

    out << "\n.section .bss\n";
    out << "_rin_buf: .zero 4096\n";

    out << "\n.section .text\n";
    out << ".global main\n\n";

    out << GAS_RUNTIME << "\n";

    for (auto& d : prog->declarations)
        if (auto* f = dynamic_cast<FunDeclStmt*>(d.get()))
            if (f->name != "main")
                emitFun(f);

    if (hasIrisMain) {
        for (auto& d : prog->declarations)
            if (auto* f = dynamic_cast<FunDeclStmt*>(d.get()))
                if (f->name == "main")
                    emitFun(f);
    } else {
        emitCMain(prog);
    }
};