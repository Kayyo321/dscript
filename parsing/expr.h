//
// Created by sullivanb on 2/20/26.
//

#ifndef DSCRIPT_EXPR_H
#define DSCRIPT_EXPR_H

#include "stmt.h"

#include <memory>
#include <utility>
#include <vector>

class AssignExpr : public Expr {
public:
    AssignExpr(Token name, ExprPtr value)
        : name(std::move(name)), value(std::move(value)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_assign_expr(this);
    }

    Token name;
    ExprPtr value;
};


class BinaryExpr : public Expr {
public:
    BinaryExpr(ExprPtr left, Token op, ExprPtr right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_binary_expr(this);
    }

    ExprPtr left;
    Token op;
    ExprPtr right;
};


class CallExpr : public Expr {
public:
    CallExpr(ExprPtr callee, Token paren, std::vector<ExprPtr> arguments)
        : callee(std::move(callee)), paren(std::move(paren)), arguments(std::move(arguments)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_call_expr(this);
    }

    ExprPtr callee;
    Token paren;
    std::vector<ExprPtr> arguments;
};


class FunctionExpr : public Expr {
public:
    explicit FunctionExpr(std::vector<StmtPtr> body)
        : body(std::move(body)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_function_expr(this);
    }

    std::vector<StmtPtr> body;
};


class GetExpr : public Expr {
public:
    GetExpr(ExprPtr obj, Token name)
        : obj(std::move(obj)), name(std::move(name)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_get_expr(this);
    }

    ExprPtr obj;
    Token name;
};


class GroupingExpr : public Expr {
public:
    explicit GroupingExpr(ExprPtr expression)
        : expression(std::move(expression)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_grouping_expr(this);
    }

    ExprPtr expression;
};


class LiteralExpr : public Expr {
public:
    explicit LiteralExpr(Value value)
        : value(std::move(value)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_literal_expr(this);
    }

    Value value;
};


class LogicalExpr : public Expr {
public:
    LogicalExpr(ExprPtr left, Token op, ExprPtr right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_logical_expr(this);
    }

    ExprPtr left;
    Token op;
    ExprPtr right;
};


class SetExpr : public Expr {
public:
    SetExpr(ExprPtr obj, Token name, ExprPtr value)
        : obj(std::move(obj)), name(std::move(name)), value(std::move(value)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_set_expr(this);
    }

    ExprPtr obj;
    Token name;
    ExprPtr value;
};


class SuperExpr : public Expr {
public:
    SuperExpr(Token keyword, Token method)
        : keyword(std::move(keyword)), method(std::move(method)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_super_expr(this);
    }

    Token keyword;
    Token method;
};


class SelfExpr : public Expr {
public:
    explicit SelfExpr(Token keyword)
        : keyword(std::move(keyword)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_self_expr(this);
    }

    Token keyword;
};


class UnaryExpr : public Expr {
public:
    UnaryExpr(Token op, ExprPtr right)
        : op(std::move(op)), right(std::move(right)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_unary_expr(this);
    }

    Token op;
    ExprPtr right;
};


class VariableExpr : public Expr {
public:
    explicit VariableExpr(Token name)
        : name(std::move(name)) {}

    Value accept(AVisitor *visitor) override {
        return visitor->visit_variable_expr(this);
    }

    Token name;
};

#endif //DSCRIPT_EXPR_H