#include "lexer.h"

#include <cctype>
#include <unordered_map>

Lexer::Lexer(const std::string& src) : src(src), pos(0), line(1) {}

char Lexer::current() const {
    return pos < src.size() ? src[pos] : '\0';
}

char Lexer::peek(int offset) const {
    size_t i = pos + offset;
    return i < src.size() ? src[i] : '\0';
}

char Lexer::advance() {
    char c = src[pos++];
    if (c == '\n') line++;
    return c;
}

Token Lexer::makeToken(TokenType t, const std::string& lexeme) const {
    return {t, lexeme, line};
}

void Lexer::skipWhitespace() {
    while (pos < src.size() && std::isspace((unsigned char)current()))
        advance();
}

Token Lexer::readNumber() {
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

Token Lexer::readString() {
    advance();
    size_t start = pos;
    while (pos < src.size() && current() != '"') advance();
    std::string content = src.substr(start, pos - start);
    if (current() == '"') advance();
    return makeToken(TokenType::STRING_LIT, content);
}

Token Lexer::readIdentOrKeyword() {
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

Token Lexer::readOperatorOrPunct() {
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

std::vector<Token> Lexer::tokenize() {
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
