#pragma once

#include "token.h"
#include "ast.h"
#include <vector>
#include <memory>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<Program> parse();

private:
    std::vector<Token> tokens;
    size_t pos;

    const Token& current() const;
    const Token& peek(int offset = 1) const;
    Token consume();

    bool check(TokenType t) const;
    bool match(TokenType t);
    Token expect(TokenType t, const std::string& msg);

    NodePtr parsePrimary();
    NodePtr parseUnary();
    NodePtr parseMulDiv();
    NodePtr parseAddSub();
    NodePtr parseExpr();
    NodePtr parseBlock();
    NodePtr parseVarDecl();
    NodePtr parseReturnStmt();
    NodePtr parseExprStmt();
    NodePtr parseStatement();
    NodePtr parseFunDecl();
    NodePtr parseDeclaration();
};
