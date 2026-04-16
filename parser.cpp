#include "parser.h"

#include <iostream>
#include <cstdlib>

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)), pos(0) {}

const Token& Parser::current() const {
    return tokens[pos];
}

const Token& Parser::peek(int offset) const {
    size_t i = pos + offset;
    return i < tokens.size() ? tokens[i] : tokens.back();
}

Token Parser::consume() {
    return tokens[pos++];
}

bool Parser::check(TokenType t) const {
    return current().type == t;
}

bool Parser::match(TokenType t) {
    if (check(t)) { pos++; return true; }
    return false;
}

Token Parser::expect(TokenType t, const std::string& msg) {
    if (!check(t)) {
        std::cerr << "Error line " << current().line << ": " << msg
                  << " (got '" << current().lexeme << "')\n";
        std::exit(1);
    }
    return consume();
}

NodePtr Parser::parsePrimary() {
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

NodePtr Parser::parseUnary() {
    return parsePrimary();
}

NodePtr Parser::parseMulDiv() {
    NodePtr left = parseUnary();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::SLASH_SLASH)) {
        std::string op = consume().lexeme;
        NodePtr right = parseUnary();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

NodePtr Parser::parseAddSub() {
    NodePtr left = parseMulDiv();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        std::string op = consume().lexeme;
        NodePtr right = parseMulDiv();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

NodePtr Parser::parseExpr() {
    return parseAddSub();
}

NodePtr Parser::parseBlock() {
    expect(TokenType::LBRACE, "expected '{'");
    auto block = std::make_unique<BlockStmt>();
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE))
        block->stmts.push_back(parseDeclaration());
    expect(TokenType::RBRACE, "expected '}'");
    return block;
}

NodePtr Parser::parseVarDecl() {
    consume();
    Token name = expect(TokenType::IDENTIFIER, "expected variable name");
    expect(TokenType::EQUALS, "expected '=' after variable name");
    NodePtr init = parseExpr();
    expect(TokenType::SEMICOLON, "expected ';' after variable declaration");
    return std::make_unique<VarDeclStmt>(name.lexeme, std::move(init));
}

NodePtr Parser::parseReturnStmt() {
    consume();
    NodePtr val;
    if (!check(TokenType::SEMICOLON))
        val = parseExpr();
    expect(TokenType::SEMICOLON, "expected ';' after return");
    return std::make_unique<ReturnStmt>(std::move(val));
}

NodePtr Parser::parseExprStmt() {
    NodePtr expr = parseExpr();
    expect(TokenType::SEMICOLON, "expected ';' after expression");
    return std::make_unique<ExprStmt>(std::move(expr));
}

NodePtr Parser::parseStatement() {
    if (check(TokenType::KW_VAR))    return parseVarDecl();
    if (check(TokenType::KW_RETURN)) return parseReturnStmt();
    if (check(TokenType::LBRACE))    return parseBlock();
    return parseExprStmt();
}

NodePtr Parser::parseFunDecl() {
    consume();
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

NodePtr Parser::parseDeclaration() {
    if (check(TokenType::KW_FUN)) return parseFunDecl();
    return parseStatement();
}

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    while (!check(TokenType::END_OF_FILE))
        program->declarations.push_back(parseDeclaration());
    return program;
}
