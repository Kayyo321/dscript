//
// Created by sullivanb on 2/20/26.
//

#ifndef DSCRIPT_A_VISITOR_H
#define DSCRIPT_A_VISITOR_H

#include "../runtime/value.h"

class BlockStmt;
class ClassStmt;
class ExpressionStmt;
class FunctionStmt;
class IfStmt;
class LogStmt;
class ReturnStmt;
class LetStmt;
class WhileStmt;
class ForStmt;
class DefStmt;

class AssignExpr;
class BinaryExpr;
class CallExpr;
class FunctionExpr;
class GetExpr;
class GroupingExpr;
class LiteralExpr;
class LogicalExpr;
class SetExpr;
class SuperExpr;
class SelfExpr;
class UnaryExpr;
class VariableExpr;

class AVisitor {
public:
    virtual ~AVisitor() = default;

    virtual Value visit_block_stmt(BlockStmt *stmt) = 0;
    virtual Value visit_class_stmt(ClassStmt *stmt) = 0;
    virtual Value visit_expression_stmt(ExpressionStmt *stmt) = 0;
    virtual Value visit_function_stmt(FunctionStmt *stmt) = 0;
    virtual Value visit_if_stmt(IfStmt *stmt) = 0;
    virtual Value visit_log_stmt(LogStmt *stmt) = 0;
    virtual Value visit_return_stmt(ReturnStmt *stmt) = 0;
    virtual Value visit_let_stmt(LetStmt *stmt) = 0;
    virtual Value visit_while_stmt(WhileStmt *stmt) = 0;
    virtual Value visit_for_stmt(ForStmt *stmt) = 0;
    virtual Value visit_def_stmt(DefStmt *stmt) = 0;

    virtual Value visit_assign_expr(AssignExpr *expr) = 0;
    virtual Value visit_binary_expr(BinaryExpr *expr) = 0;
    virtual Value visit_call_expr(CallExpr *expr) = 0;
    virtual Value visit_function_expr(FunctionExpr *expr) = 0;
    virtual Value visit_get_expr(GetExpr *expr) = 0;
    virtual Value visit_grouping_expr(GroupingExpr *expr) = 0;
    virtual Value visit_literal_expr(LiteralExpr *expr) = 0;
    virtual Value visit_logical_expr(LogicalExpr *expr) = 0;
    virtual Value visit_set_expr(SetExpr *expr) = 0;
    virtual Value visit_super_expr(SuperExpr *expr) = 0;
    virtual Value visit_self_expr(SelfExpr *expr) = 0;
    virtual Value visit_unary_expr(UnaryExpr *expr) = 0;
    virtual Value visit_variable_expr(VariableExpr *expr) = 0;
};

#endif //DSCRIPT_A_VISITOR_H