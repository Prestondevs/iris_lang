/*
Authored by: Preston Vardaman
Iris Compiler Project
file type: .ir
*/
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include <cstdlib>

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

static const char* RUNTIME = R"(
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef enum { IRIS_INT = 0, IRIS_FLOAT = 1, IRIS_STR = 2 } IrisType;
typedef struct { IrisType type; union { int64_t i; double f; const char* s; }; } Value;

static Value _make_int(int64_t v)    { Value r; r.type = IRIS_INT;   r.i = v; return r; }
static Value _make_float(double v)   { Value r; r.type = IRIS_FLOAT; r.f = v; return r; }
static Value _make_str(const char* v){ Value r; r.type = IRIS_STR;   r.s = v; return r; }

static double   _to_float(Value v){ return v.type == IRIS_FLOAT ? v.f : (double)v.i; }
static int64_t  _to_int(Value v)  { return v.type == IRIS_INT   ? v.i : (int64_t)v.f; }

static Value _iris_rt_add(Value a, Value b){
    if(a.type==IRIS_FLOAT||b.type==IRIS_FLOAT) return _make_float(_to_float(a)+_to_float(b));
    return _make_int(a.i+b.i);
}
static Value _iris_rt_sub(Value a, Value b){
    if(a.type==IRIS_FLOAT||b.type==IRIS_FLOAT) return _make_float(_to_float(a)-_to_float(b));
    return _make_int(a.i-b.i);
}
static Value _iris_rt_mul(Value a, Value b){
    if(a.type==IRIS_FLOAT||b.type==IRIS_FLOAT) return _make_float(_to_float(a)*_to_float(b));
    return _make_int(a.i*b.i);
}
static Value _iris_rt_div(Value a, Value b){ return _make_float(_to_float(a)/_to_float(b)); }
static Value _iris_rt_idiv(Value a, Value b){ return _make_int(_to_int(a)/_to_int(b)); }

static Value _iris_rout(Value v){
    if(v.type==IRIS_INT)   printf("%lld\n",(long long)v.i);
    else if(v.type==IRIS_FLOAT) printf("%g\n",v.f);
    else if(v.type==IRIS_STR)   printf("%s\n",v.s);
    return _make_int(0);
}

static Value _iris_rin(void){
    static char buf[4096];
    if(fgets(buf,sizeof(buf),stdin)){
        size_t len=strlen(buf);
        if(len>0&&buf[len-1]=='\n') buf[len-1]='\0';
        return _make_str(buf);
    }
    return _make_str("");
}
)";

class CodeGen {
    std::ostream& out;
    bool hasIrisMain = false;

    void emitExpr(ASTNode* node) {
        if (auto* n = dynamic_cast<IntLitExpr*>(node)) {
            out << "_make_int(" << n->value << "LL)";
        } else if (auto* n = dynamic_cast<FloatLitExpr*>(node)) {
            out << "_make_float(" << n->value << ")";
        } else if (auto* n = dynamic_cast<StringLitExpr*>(node)) {
            out << "_make_str(\"" << n->value << "\")";
        } else if (auto* n = dynamic_cast<IdentExpr*>(node)) {
            out << n->name;
        } else if (auto* n = dynamic_cast<BinaryExpr*>(node)) {
            const char* fn =
                n->op == "+"  ? "_iris_rt_add"  :
                n->op == "-"  ? "_iris_rt_sub"  :
                n->op == "*"  ? "_iris_rt_mul"  :
                n->op == "/"  ? "_iris_rt_div"  :
                n->op == "//" ? "_iris_rt_idiv" : "_iris_rt_add";
            out << fn << "(";
            emitExpr(n->left.get());
            out << ", ";
            emitExpr(n->right.get());
            out << ")";
        } else if (auto* n = dynamic_cast<CallExpr*>(node)) {
            out << "_iris_" << n->callee << "(";
            for (size_t i = 0; i < n->args.size(); i++) {
                if (i) out << ", ";
                emitExpr(n->args[i].get());
            }
            out << ")";
        }
    }

    void emitStmt(ASTNode* node, int indent) {
        std::string p(indent * 4, ' ');
        if (auto* n = dynamic_cast<VarDeclStmt*>(node)) {
            out << p << "Value " << n->name << " = ";
            emitExpr(n->initializer.get());
            out << ";\n";
        } else if (auto* n = dynamic_cast<ReturnStmt*>(node)) {
            out << p << "return ";
            if (n->value) emitExpr(n->value.get());
            else          out << "_make_int(0)";
            out << ";\n";
        } else if (auto* n = dynamic_cast<ExprStmt*>(node)) {
            out << p;
            emitExpr(n->expr.get());
            out << ";\n";
        } else if (auto* n = dynamic_cast<BlockStmt*>(node)) {
            out << p << "{\n";
            for (auto& s : n->stmts) emitStmt(s.get(), indent + 1);
            out << p << "}\n";
        }
    }

    void emitFun(FunDeclStmt* f) {
        out << "Value _iris_" << f->name << "(";
        for (size_t i = 0; i < f->params.size(); i++) {
            if (i) out << ", ";
            out << "Value " << f->params[i];
        }
        if (f->params.empty()) out << "void";
        out << ") {\n";
        auto* block = dynamic_cast<BlockStmt*>(f->body.get());
        if (block)
            for (auto& s : block->stmts) emitStmt(s.get(), 1);
        out << "    return _make_int(0);\n";
        out << "}\n\n";
    }

public:
    explicit CodeGen(std::ostream& out) : out(out) {}

    void generate(Program* prog) {
        for (auto& d : prog->declarations)
            if (auto* f = dynamic_cast<FunDeclStmt*>(d.get()))
                if (f->name == "main") { hasIrisMain = true; break; }

        out << RUNTIME << "\n";

        for (auto& d : prog->declarations) {
            if (auto* f = dynamic_cast<FunDeclStmt*>(d.get())) {
                out << "Value _iris_" << f->name << "(";
                for (size_t i = 0; i < f->params.size(); i++) {
                    if (i) out << ", ";
                    out << "Value";
                }
                if (f->params.empty()) out << "void";
                out << ");\n";
            }
        }
        out << "\n";

        for (auto& d : prog->declarations)
            if (auto* f = dynamic_cast<FunDeclStmt*>(d.get()))
                emitFun(f);

        out << "int main(void) {\n";
        for (auto& d : prog->declarations)
            if (!dynamic_cast<FunDeclStmt*>(d.get()))
                emitStmt(d.get(), 1);
        if (hasIrisMain)
            out << "    _iris_main();\n";
        out << "    return 0;\n}\n";
    }
};

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

    std::ostringstream cSrc;
    CodeGen codegen(cSrc);
    codegen.generate(program.get());

    std::string tmpC = outputName + ".iris.c";
    {
        std::ofstream tmp(tmpC);
        if (!tmp) {
            std::cerr << "Cannot write temp file: " << tmpC << "\n";
            return 1;
        }
        tmp << cSrc.str();
    }

    std::string cmd = "cc -o \"" + outputName + "\" \"" + tmpC + "\"";
    int ret = std::system(cmd.c_str());
    std::remove(tmpC.c_str());

    if (ret != 0) {
        std::cerr << "Compilation failed\n";
        return 1;
    }

    return 0;
}
