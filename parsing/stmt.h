//
// Created by sullivanb on 2/20/26.
//

#ifndef DSCRIPT_STMT_H
#define DSCRIPT_STMT_H

#include <optional>
#include <utility>
#include <vector>

#include "a_visitor.h"
#include "../lexing/token.h"

class Stmt {
public:
    virtual ~Stmt() = default;

    virtual Value accept(AVisitor *visitor) = 0;
};

struct Expr: Stmt {
};

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

class BlockStmt : public Stmt {
public:
    explicit BlockStmt(std::vector<StmtPtr> statements)
        : statements(std::move(statements)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_block_stmt(this);
    }

    std::vector<StmtPtr> statements;
};

class ClassStmt : public Stmt {
public:
    ClassStmt(Token name, ExprPtr superStruct, std::vector<std::shared_ptr<FunctionStmt>> methods, bool isStatic, int status)
        : name(std::move(name)), super_class(std::move(superStruct)), methods(std::move(methods)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_class_stmt(this);
    }

    Token name;
    ExprPtr super_class;
    std::vector<std::shared_ptr<FunctionStmt>> methods;
};

class ExpressionStmt : public Stmt {
public:
    explicit ExpressionStmt(ExprPtr expression)
        : expression(std::move(expression)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_expression_stmt(this);
    }

    ExprPtr expression;
}; 

class BlockLiteral {
public:
    explicit BlockLiteral(Token name) : name{std::move(name)}, expect{std::nullopt} {}
    BlockLiteral(Token name, Token expect) : name{std::move(name)}, expect{expect} {}
    
    Token name;
    std::optional<Token> expect; 
};

class FunctionStmt : public Stmt {
public:
    FunctionStmt(Token name, std::vector<Token> params, std::vector<StmtPtr> body, bool isStatic)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)) {}
    
    FunctionStmt(Token name, std::vector<Token> params, std::vector<BlockLiteral> blocks, std::vector<StmtPtr> body, bool isStatic)
        : name(std::move(name)), params(std::move(params)), blocks{std::move(blocks)}, body(std::move(body)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_function_stmt(this);
    }

    Token name;
    std::vector<Token> params;
    std::optional<std::vector<BlockLiteral>> blocks;
    std::vector<StmtPtr> body;
};

class IfStmt : public Stmt {
public:
    IfStmt(ExprPtr condition, StmtPtr thenBranch, StmtPtr elseBranch)
        : condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_if_stmt(this);
    }

    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;
};

class LogStmt : public Stmt {
public:
    explicit LogStmt(ExprPtr expression)
        : expression(std::move(expression)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_log_stmt(this);
    }

    ExprPtr expression;
};

class ReturnStmt : public Stmt {
public:
    ReturnStmt(Token keyword, ExprPtr value)
        : keyword(std::move(keyword)), value(std::move(value)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_return_stmt(this);
    }

    Token keyword;
    ExprPtr value;
};

class LetStmt : public Stmt {
public:
    LetStmt(Token keyword, Token name, ExprPtr init)
        : keyword(std::move(keyword)), name(std::move(name)), init(std::move(init)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_let_stmt(this);
    }

    Token keyword;
    Token name;
    ExprPtr init;
};

class WhileStmt : public Stmt {
public:
    WhileStmt(ExprPtr condition, StmtPtr body, StmtPtr finally_clause)
        : condition(std::move(condition)), body(std::move(body)), finally_clause(std::move(finally_clause)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_while_stmt(this);
    }

    ExprPtr condition;
    StmtPtr body;
    StmtPtr finally_clause;
};

class ForStmt : public Stmt {
public:
    ForStmt(StmtPtr init, ExprPtr condition, ExprPtr inc, StmtPtr body, StmtPtr finally_clause)
        : init(std::move(init)), condition(std::move(condition)), inc(std::move(inc)), body(std::move(body)), finally_clause(std::move(finally_clause)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_for_stmt(this);
    }

    StmtPtr init;
    ExprPtr condition;
    ExprPtr inc;
    StmtPtr body;
    StmtPtr finally_clause;
};

class DefStmt : public Stmt {
public:
    DefStmt(Token name, std::vector<Token> params, std::vector<StmtPtr> body, bool isStatic)
        : new_keyword(std::move(name)), params(std::move(params)), body(std::move(body)) {}
    
    DefStmt(Token name, std::vector<Token> params, std::vector<BlockLiteral> blocks, std::vector<StmtPtr> body, bool isStatic)
        : new_keyword(std::move(name)), params(std::move(params)), blocks{std::move(blocks)}, body(std::move(body)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_def_stmt(this);
    }

    Token new_keyword;
    std::vector<Token> params;
    std::optional<std::vector<BlockLiteral>> blocks;
    std::vector<StmtPtr> body;
};

#endif //DSCRIPT_STMT_H