/*
Authored by: Preston Vardaman
Iris Compiler Project
file type: .ir
*/

#ifdef _WIN32
    #define popen _popen
    #define pclose _pclose
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <cstdio>


enum class TokenType {
    INT_LIT, FLOAT_LIT, STRING_LIT,
    IDENTIFIER, KW_VAR, KW_FUN, KW_RETURN,
    PLUS, MINUS, STAR, SLASH, SLASH_SLASH,
    LPAREN, RPAREN, LBRACE, RBRACE, SEMICOLON, EQUALS, COMMA,
    END_OF_FILE, UNKNOWN
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
};

class Lexer {
    const std::string& src;
    size_t pos;
    int line;

    char current() const {
        return pos < src.size() ? src[pos] : '\0';
    }
    char peek(int offset = 1) const {
        size_t i = pos + offset;
        return i < src.size() ? src[i] : '\0';
    }
    char advance() {
        char c = src[pos++];
        if (c == '\n') line++;
        return c;
    }
    Token makeToken(TokenType t, const std::string& lexeme) const {
        return {t, lexeme, line};
    }

    void skipWhitespace() {
        while (pos < src.size() && std::isspace((unsigned char)current()))
            advance();
    }

    Token readNumber() {
        size_t start = pos;
        while (std::isdigit((unsigned char)current())) advance();
        bool isFloat = false;
        if (current() == '.' && std::isdigit((unsigned char)peek())) {
            isFloat = true;
            advance();
            while (std::isdigit((unsigned char)current())) advance();
        }
        std::string lex = src.substr(start, pos - start);
        return makeToken(isFloat ? TokenType::FLOAT_LIT : TokenType::INT_LIT, lex);
    }

    Token readString() {
        advance(); // opening "
        size_t start = pos;
        while (pos < src.size() && current() != '"') advance();
        std::string content = src.substr(start, pos - start);
        if (current() == '"') advance(); // closing "
        return makeToken(TokenType::STRING_LIT, content);
    }

    Token readIdentOrKeyword() {
        static const std::unordered_map<std::string, TokenType> keywords = {
            {"var",    TokenType::KW_VAR},
            {"fun",    TokenType::KW_FUN},
            {"return", TokenType::KW_RETURN},
        };
        size_t start = pos;
        while (std::isalnum((unsigned char)current()) || current() == '_') advance();
        std::string lex = src.substr(start, pos - start);
        auto it = keywords.find(lex);
        TokenType t = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
        return makeToken(t, lex);
    }

    Token readOperatorOrPunct() {
        char c = advance();
        switch (c) {
            case '+': return makeToken(TokenType::PLUS,      "+");
            case '-': return makeToken(TokenType::MINUS,     "-");
            case '*': return makeToken(TokenType::STAR,      "*");
            case '/':
                if (current() == '/') { advance(); return makeToken(TokenType::SLASH_SLASH, "//"); }
                return makeToken(TokenType::SLASH, "/");
            case '(': return makeToken(TokenType::LPAREN,    "(");
            case ')': return makeToken(TokenType::RPAREN,    ")");
            case '{': return makeToken(TokenType::LBRACE,    "{");
            case '}': return makeToken(TokenType::RBRACE,    "}");
            case ';': return makeToken(TokenType::SEMICOLON, ";");
            case '=': return makeToken(TokenType::EQUALS,    "=");
            case ',': return makeToken(TokenType::COMMA,     ",");
            default:  return makeToken(TokenType::UNKNOWN,   std::string(1, c));
        }
    }

public:
    explicit Lexer(const std::string& src) : src(src), pos(0), line(1) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (true) {
            skipWhitespace();
            if (pos >= src.size()) {
                tokens.push_back(makeToken(TokenType::END_OF_FILE, ""));
                break;
            }
            char c = current();
            if (std::isdigit((unsigned char)c))
                tokens.push_back(readNumber());
            else if (c == '"')
                tokens.push_back(readString());
            else if (std::isalpha((unsigned char)c) || c == '_')
                tokens.push_back(readIdentOrKeyword());
            else
                tokens.push_back(readOperatorOrPunct());
        }
        return tokens;
    }
};

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual void print(int indent = 0) const = 0;
protected:
    static std::string pad(int indent) { return std::string(indent * 2, ' '); }
};
using NodePtr = std::unique_ptr<ASTNode>;

struct IntLitExpr : ASTNode {
    int64_t value;
    explicit IntLitExpr(int64_t v) : value(v) {}
    void print(int i) const override {
        std::cout << pad(i) << "IntLit(" << value << ")\n";
    }
};

struct FloatLitExpr : ASTNode {
    double value;
    explicit FloatLitExpr(double v) : value(v) {}
    void print(int i) const override {
        std::cout << pad(i) << "FloatLit(" << value << ")\n";
    }
};

struct StringLitExpr : ASTNode {
    std::string value;
    explicit StringLitExpr(std::string v) : value(std::move(v)) {}
    void print(int i) const override {
        std::cout << pad(i) << "StringLit(\"" << value << "\")\n";
    }
};

struct IdentExpr : ASTNode {
    std::string name;
    explicit IdentExpr(std::string n) : name(std::move(n)) {}
    void print(int i) const override {
        std::cout << pad(i) << "Ident(" << name << ")\n";
    }
};

struct BinaryExpr : ASTNode {
    std::string op;
    NodePtr left, right;
    BinaryExpr(std::string op, NodePtr l, NodePtr r)
        : op(std::move(op)), left(std::move(l)), right(std::move(r)) {}
    void print(int i) const override {
        std::cout << pad(i) << "Binary(" << op << ")\n";
        left->print(i + 1);
        right->print(i + 1);
    }
};

struct CallExpr : ASTNode {
    std::string callee;
    std::vector<NodePtr> args;
    CallExpr(std::string callee, std::vector<NodePtr> args)
        : callee(std::move(callee)), args(std::move(args)) {}
    void print(int i) const override {
        std::cout << pad(i) << "Call(" << callee << ")\n";
        for (auto& a : args) a->print(i + 1);
    }
};

struct VarDeclStmt : ASTNode {
    std::string name;
    NodePtr initializer;
    VarDeclStmt(std::string name, NodePtr init)
        : name(std::move(name)), initializer(std::move(init)) {}
    void print(int i) const override {
        std::cout << pad(i) << "VarDecl(" << name << ")\n";
        initializer->print(i + 1);
    }
};

struct ReturnStmt : ASTNode {
    NodePtr value;
    explicit ReturnStmt(NodePtr v) : value(std::move(v)) {}
    void print(int i) const override {
        std::cout << pad(i) << "Return\n";
        if (value) value->print(i + 1);
    }
};

struct ExprStmt : ASTNode {
    NodePtr expr;
    explicit ExprStmt(NodePtr e) : expr(std::move(e)) {}
    void print(int i) const override {
        std::cout << pad(i) << "ExprStmt\n";
        expr->print(i + 1);
    }
};

struct BlockStmt : ASTNode {
    std::vector<NodePtr> stmts;
    void print(int i) const override {
        std::cout << pad(i) << "Block\n";
        for (auto& s : stmts) s->print(i + 1);
    }
};

struct FunDeclStmt : ASTNode {
    std::string name;
    std::vector<std::string> params;
    NodePtr body;
    FunDeclStmt(std::string name, std::vector<std::string> params, NodePtr body)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)) {}
    void print(int i) const override {
        std::cout << pad(i) << "FunDecl(" << name << ")(";
        for (size_t j = 0; j < params.size(); j++) {
            if (j) std::cout << ", ";
            std::cout << params[j];
        }
        std::cout << ")\n";
        body->print(i + 1);
    }
};

struct Program : ASTNode {
    std::vector<NodePtr> declarations;
    void print(int i) const override {
        std::cout << pad(i) << "Program\n";
        for (auto& d : declarations) d->print(i + 1);
    }
};

class Parser {
    std::vector<Token> tokens;
    size_t pos;

    const Token& current() const { return tokens[pos]; }
    const Token& peek(int offset = 1) const {
        size_t i = pos + offset;
        return i < tokens.size() ? tokens[i] : tokens.back();
    }
    Token consume() { return tokens[pos++]; }

    bool check(TokenType t) const { return current().type == t; }
    bool match(TokenType t) {
        if (check(t)) { pos++; return true; }
        return false;
    }
    Token expect(TokenType t, const std::string& msg) {
        if (!check(t)) {
            std::cerr << "Error line " << current().line << ": " << msg
                      << " (got '" << current().lexeme << "')\n";
            std::exit(1);
        }
        return consume();
    }

    NodePtr parsePrimary() {
        Token tok = current();

        if (tok.type == TokenType::INT_LIT) {
            consume();
            return std::make_unique<IntLitExpr>(std::stoll(tok.lexeme));
        }
        if (tok.type == TokenType::FLOAT_LIT) {
            consume();
            return std::make_unique<FloatLitExpr>(std::stod(tok.lexeme));
        }
        if (tok.type == TokenType::STRING_LIT) {
            consume();
            return std::make_unique<StringLitExpr>(tok.lexeme);
        }
        if (tok.type == TokenType::IDENTIFIER) {
            consume();
            if (check(TokenType::LPAREN)) {
                consume();
                std::vector<NodePtr> args;
                while (!check(TokenType::RPAREN) && !check(TokenType::END_OF_FILE)) {
                    args.push_back(parseExpr());
                    if (!match(TokenType::COMMA)) break;
                }
                expect(TokenType::RPAREN, "expected ')' after call args");
                return std::make_unique<CallExpr>(tok.lexeme, std::move(args));
            }
            return std::make_unique<IdentExpr>(tok.lexeme);
        }
        if (tok.type == TokenType::LPAREN) {
            consume();
            NodePtr inner = parseExpr();
            expect(TokenType::RPAREN, "expected ')' after expression");
            return inner;
        }

        std::cerr << "Error line " << tok.line << ": unexpected token '" << tok.lexeme << "'\n";
        std::exit(1);
    }

    NodePtr parseUnary() { return parsePrimary(); }

    NodePtr parseMulDiv() {
        NodePtr left = parseUnary();
        while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::SLASH_SLASH)) {
            std::string op = consume().lexeme;
            NodePtr right = parseUnary();
            left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        }
        return left;
    }

    NodePtr parseAddSub() {
        NodePtr left = parseMulDiv();
        while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
            std::string op = consume().lexeme;
            NodePtr right = parseMulDiv();
            left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        }
        return left;
    }

    NodePtr parseExpr() { return parseAddSub(); }

    NodePtr parseBlock() {
        expect(TokenType::LBRACE, "expected '{'");
        auto block = std::make_unique<BlockStmt>();
        while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE))
            block->stmts.push_back(parseDeclaration());
        expect(TokenType::RBRACE, "expected '}'");
        return block;
    }

    NodePtr parseVarDecl() {
        consume(); // var
        Token name = expect(TokenType::IDENTIFIER, "expected variable name");
        expect(TokenType::EQUALS, "expected '=' after variable name");
        NodePtr init = parseExpr();
        expect(TokenType::SEMICOLON, "expected ';' after variable declaration");
        return std::make_unique<VarDeclStmt>(name.lexeme, std::move(init));
    }

    NodePtr parseReturnStmt() {
        consume(); // return
        NodePtr val;
        if (!check(TokenType::SEMICOLON))
            val = parseExpr();
        expect(TokenType::SEMICOLON, "expected ';' after return");
        return std::make_unique<ReturnStmt>(std::move(val));
    }

    NodePtr parseExprStmt() {
        NodePtr expr = parseExpr();
        expect(TokenType::SEMICOLON, "expected ';' after expression");
        return std::make_unique<ExprStmt>(std::move(expr));
    }

    NodePtr parseStatement() {
        if (check(TokenType::KW_VAR))    return parseVarDecl();
        if (check(TokenType::KW_RETURN)) return parseReturnStmt();
        if (check(TokenType::LBRACE))    return parseBlock();
        return parseExprStmt();
    }

    NodePtr parseFunDecl() {
        consume(); // fun
        Token name = expect(TokenType::IDENTIFIER, "expected function name");
        expect(TokenType::LPAREN, "expected '(' after function name");
        std::vector<std::string> params;
        while (!check(TokenType::RPAREN) && !check(TokenType::END_OF_FILE)) {
            Token p = expect(TokenType::IDENTIFIER, "expected parameter name");
            params.push_back(p.lexeme);
            if (!match(TokenType::COMMA)) break;
        }
        expect(TokenType::RPAREN, "expected ')' after parameters");
        NodePtr body = parseBlock();
        return std::make_unique<FunDeclStmt>(name.lexeme, std::move(params), std::move(body));
    }

    NodePtr parseDeclaration() {
        if (check(TokenType::KW_FUN)) return parseFunDecl();
        return parseStatement();
    }

public:
    explicit Parser(std::vector<Token> tokens) : tokens(std::move(tokens)), pos(0) {}

    std::unique_ptr<Program> parse() {
        auto program = std::make_unique<Program>();
        while (!check(TokenType::END_OF_FILE))
            program->declarations.push_back(parseDeclaration());
        return program;
    }
};

// x86-64 GAS Intel-syntax Assembly Code Generator
// Value convention: type tag in rax (0=INT,1=FLOAT,2=STR), data in rdx (raw 64-bit)
// Stack slots: 16 bytes each — type at [rbp+off], data at [rbp+off+8]

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

class AsmCodeGen {
    std::ostream& out;

    std::vector<std::string> floatLiterals;
    std::vector<std::string> stringLiterals;
    std::unordered_map<std::string, int> floatLitIdx;
    std::unordered_map<std::string, int> strLitIdx;

    // Per-function state (reset each emitFun / emitCMain)
    std::unordered_map<std::string, int> locals; // name -> rbp offset of type field
    int localCount = 0;
    int tempDepth  = 0; // current temp slot depth (0-based, max 7)
    bool hasIrisMain = false;

    // Slot k: type at [rbp + slotOffset(k)], data at [rbp + slotOffset(k) + 8]
    static int slotOffset(int k) { return -16 * (k + 1); }
    int tempSlotOffset(int d) const { return slotOffset(localCount + d); }

    static std::string fmtOff(int off) {
        if (off < 0) return " - " + std::to_string(-off);
        if (off > 0) return " + " + std::to_string(off);
        return "";
    }

    // Produce NASM db argument(s) for a string (without null terminator)
    static std::string nasmStrLit(const std::string& s) {
        if (s.empty()) return "0"; // just null terminator byte
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

    int getFloatIdx(double v) {
        // Store as raw hex bits so NASM always treats it as IEEE754 double
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

    int getStrIdx(const std::string& s) {
        auto it = strLitIdx.find(s);
        if (it != strLitIdx.end()) return it->second;
        int idx = (int)stringLiterals.size();
        stringLiterals.push_back(s);
        strLitIdx[s] = idx;
        return idx;
    }

    void collectLiterals(ASTNode* node) {
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

    void buildLocalsMap(FunDeclStmt* f) {
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

    void emitExpr(ASTNode* node);
    void emitStmt(ASTNode* node);

    void emitFunHeader(const std::string& label, int frameSize) {
        out << label << ":\n";
        out << "    push rbp\n";
        out << "    mov rbp, rsp\n";
        out << "    sub rsp, " << frameSize << "\n";
    }

    void emitFun(FunDeclStmt* f) {
        buildLocalsMap(f);
        int frameSize = (localCount + 8) * 16;

        std::string label = (f->name == "main") ? "main" : "_iris_" + f->name;
        emitFunHeader(label, frameSize);

        // Spill incoming params from registers to stack slots
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

    void emitCMain(Program* prog) {
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

public:
    explicit AsmCodeGen(std::ostream& out) : out(out) {}

    void generate(Program* prog) {
        for (auto& d : prog->declarations)
            if (auto* f = dynamic_cast<FunDeclStmt*>(d.get()))
                if (f->name == "main") { hasIrisMain = true; break; }

        collectLiterals(prog);

        out << ".intel_syntax noprefix\n\n";

        out << ".section .data\n";
        // Format strings as byte arrays (avoids GAS string escaping quirks)
        out << "_fmt_int: .byte 37,108,100,10,0\n"; // "%ld\n"
        out << "_fmt_flt: .byte 37,103,10,0\n";     // "%g\n"
        out << "_fmt_str: .byte 37,115,10,0\n";     // "%s\n"
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
    }
};

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
        // arrange args: right (rax:rdx) → rdx:rcx, left (from stack) → rdi:rsi
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
            // Evaluate each arg, save result to successive temp slots
            for (int i = 0; i < argCount; i++) {
                emitExpr(n->args[i].get());
                int toff = tempSlotOffset(savedDepth + i);
                out << "    mov [rbp" << fmtOff(toff)   << "], rax\n";
                out << "    mov [rbp" << fmtOff(toff+8) << "], rdx\n";
                tempDepth = savedDepth + i + 1; // protect saved slots from sub-evals
            }
            tempDepth = savedDepth;
            // Load args into calling registers
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

int main(int argc, char* argv[]) {
    if (argc != 4 || std::string(argv[1]) != "-o") {
        std::cerr << "Usage: iris -o <output> <file.ir>\n";
        return 1;
    }

    std::string outputName = argv[2];
    std::string inputFile  = argv[3];

    std::ifstream file(inputFile);
    if (!file) {
        std::cerr << "Cannot open file: " << inputFile << "\n";
        return 1;
    }
    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    auto program = parser.parse();

    std::ostringstream asmSrc;
    AsmCodeGen codegen(asmSrc);
    codegen.generate(program.get());

    // Pipe assembly directly into `as` — no intermediate .asm file on disk
    std::string tmpObj = outputName + ".iris.o";
    {
        std::string asCmd = "as --64 -o \"" + tmpObj + "\" -";
        FILE* proc = popen(asCmd.c_str(), "w");
        if (!proc) {
            std::cerr << "Cannot run assembler\n";
            return 1;
        }
        const std::string& asmStr = asmSrc.str();
        fwrite(asmStr.c_str(), 1, asmStr.size(), proc);
        if (pclose(proc) != 0) {
            std::cerr << "Assembly failed\n";
            return 1;
        }
    }

    std::string cmd = "gcc -o \"" + outputName + "\" \"" + tmpObj + "\" -no-pie";
    int ret = std::system(cmd.c_str());
    std::remove(tmpObj.c_str());

    if (ret != 0) {
        std::cerr << "Link failed\n";
        return 1;
    }

    return 0;
}
