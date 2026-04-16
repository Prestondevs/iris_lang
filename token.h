#pragma once

#include <string>

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
