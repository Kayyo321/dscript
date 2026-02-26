#ifndef RESOLVER_H
#define RESOLVER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "../parsing/expr.h"
#include "../parsing/stmt.h"

class Resolver : public AVisitor {
public:
    void resolve(const std::vector<StmtPtr> &statements);

    const std::vector<std::string> &get_errors() const;
    bool had_error() const;

    const std::unordered_map<const Expr *, int> &get_locals() const;

    Value visit_block_stmt(BlockStmt *stmt) override;
    Value visit_class_stmt(ClassStmt *stmt) override;
    Value visit_expression_stmt(ExpressionStmt *stmt) override;
    Value visit_function_stmt(FunctionStmt *stmt) override;
    Value visit_if_stmt(IfStmt *stmt) override;
    Value visit_log_stmt(LogStmt *stmt) override;
    Value visit_return_stmt(ReturnStmt *stmt) override;
    Value visit_let_stmt(LetStmt *stmt) override;
    Value visit_while_stmt(WhileStmt *stmt) override;
    Value visit_for_stmt(ForStmt *stmt) override;
    Value visit_do_while_stmt(DoWhileStmt *stmt) override;
    Value visit_break_stmt(BreakStmt *stmt) override;
    Value visit_continue_stmt(ContinueStmt *stmt) override;
    Value visit_def_stmt(DefStmt *stmt) override;
    Value visit_import_stmt(ImportStmt *stmt) override;
    Value visit_from_import_stmt(FromImportStmt *stmt) override;
    Value visit_export_stmt(ExportStmt *stmt) override;

    Value visit_assign_expr(AssignExpr *expr) override;
    Value visit_binary_expr(BinaryExpr *expr) override;
    Value visit_call_expr(CallExpr *expr) override;
    Value visit_function_expr(FunctionExpr *expr) override;
    Value visit_index_expr(IndexExpr *expr) override;
    Value visit_list_expr(ListExpr *expr) override;
    Value visit_named_block_expr(NamedBlockExpr *expr) override;
    Value visit_get_expr(GetExpr *expr) override;
    Value visit_grouping_expr(GroupingExpr *expr) override;
    Value visit_literal_expr(LiteralExpr *expr) override;
    Value visit_logical_expr(LogicalExpr *expr) override;
    Value visit_set_expr(SetExpr *expr) override;
    Value visit_super_expr(SuperExpr *expr) override;
    Value visit_self_expr(SelfExpr *expr) override;
    Value visit_unary_expr(UnaryExpr *expr) override;
    Value visit_update_expr(UpdateExpr *expr) override;
    Value visit_variable_expr(VariableExpr *expr) override;

private:
    enum class FunctionType {
        None,
        Function,
        Method,
        Initializer,
        Def,
    };

    enum class ClassType {
        None,
        Class,
        SubClass,
    };

    void resolve(const StmtPtr &statement);
    void resolve(const ExprPtr &expression);
    void resolve_function(const std::vector<Token> &params, const std::optional<std::vector<BlockLiteral>> &blocks, const std::vector<StmtPtr> &body, FunctionType type);
    void resolve_function(const std::vector<FunctionStmt::Parameter> &params, const std::optional<std::vector<BlockLiteral>> &blocks, const std::vector<StmtPtr> &body, FunctionType type);

    void begin_scope();
    void end_scope();
    void declare(const Token &name);
    void define(const Token &name);
    void resolve_local(const Expr *expr, const Token &name);

    void error_at(const Token &token, const std::string &message);

    std::vector<std::unordered_map<std::string, bool>> scopes;
    std::vector<std::string> errors;
    std::unordered_map<const Expr *, int> locals;
    FunctionType current_function{FunctionType::None};
    ClassType current_class{ClassType::None};
    int loop_depth{0};
};

#endif //RESOLVER_H
