#pragma once

#include "token.h"
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(const std::string& src);
    std::vector<Token> tokenize();

private:
    const std::string& src;
    size_t pos;
    int line;

    char current() const;
    char peek(int offset = 1) const;
    char advance();
    Token makeToken(TokenType t, const std::string& lexeme) const;

    void skipWhitespace();
    Token readNumber();
    Token readString();
    Token readIdentOrKeyword();
    Token readOperatorOrPunct();
};
