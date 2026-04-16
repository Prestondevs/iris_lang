#pragma once

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <cstdint>

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
