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
        : keyword(std::move(keyword)) {
        if (value != nullptr) {
            values.push_back(std::move(value));
        }
    }

    ReturnStmt(Token keyword, std::vector<ExprPtr> values)
        : keyword(std::move(keyword)), values(std::move(values)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_return_stmt(this);
    }

    Token keyword;
    std::vector<ExprPtr> values;
};

class LetStmt : public Stmt {
public:
    LetStmt(Token keyword, Token name, ExprPtr init)
        : keyword(std::move(keyword)), names{std::move(name)} {
        if (init != nullptr) {
            inits.push_back(std::move(init));
        }
    }

    LetStmt(Token keyword, std::vector<Token> names, std::vector<ExprPtr> inits)
        : keyword(std::move(keyword)), names(std::move(names)), inits(std::move(inits)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_let_stmt(this);
    }

    Token keyword;
    std::vector<Token> names;
    std::vector<ExprPtr> inits;
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

class ImportStmt : public Stmt {
public:
    ImportStmt(Token keyword, Token path, Token alias)
        : keyword(std::move(keyword)), path(std::move(path)), alias(std::move(alias)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_import_stmt(this);
    }

    Token keyword;
    Token path;
    Token alias;
};

struct ImportName {
    Token name;
    Token alias;
};

class FromImportStmt : public Stmt {
public:
    FromImportStmt(Token keyword, Token path, std::vector<ImportName> names)
        : keyword(std::move(keyword)), path(std::move(path)), names(std::move(names)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_from_import_stmt(this);
    }

    Token keyword;
    Token path;
    std::vector<ImportName> names;
};

class ExportStmt : public Stmt {
public:
    explicit ExportStmt(StmtPtr declaration)
        : declaration(std::move(declaration)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_export_stmt(this);
    }

    StmtPtr declaration;
};

#endif //DSCRIPT_STMT_H