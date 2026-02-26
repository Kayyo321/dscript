#ifndef DSCRIPT_JSON_BUILDER_H
#define DSCRIPT_JSON_BUILDER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "../parsing/expr.h"

class JsonAstBuilder : public AVisitor {
public:
    explicit JsonAstBuilder(const std::unordered_map<const Expr *, int> &resolved_locals);

    std::string serialize_statements(const std::vector<StmtPtr> &statements);
    std::string serialize_resolved_locals() const;

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
    std::string current_json_;
    std::unordered_map<const Expr *, int> resolved_locals_;
    std::unordered_map<const Expr *, std::size_t> expr_ids_;
    std::size_t next_expr_id_;
    std::size_t current_expr_id_;

    static std::string json_escape(const std::string &value);
    static std::string serialize_token(const Token &token);
    static std::string serialize_value(const Value &value);

    std::size_t ensure_expr_id(const Expr *expr);
    std::string serialize_stmt(const StmtPtr &stmt);
    std::string serialize_expr(const ExprPtr &expr);
    static std::string serialize_stmt_array(const std::vector<StmtPtr> &stmts, JsonAstBuilder &builder);
    static std::string serialize_expr_array(const std::vector<ExprPtr> &exprs, JsonAstBuilder &builder);
    static std::string serialize_params(const std::vector<FunctionStmt::Parameter> &params);
    static std::string serialize_class_fields(const std::vector<ClassField> &fields, JsonAstBuilder &builder);
    static std::string serialize_block_literals(const std::optional<std::vector<BlockLiteral>> &blocks);
    static std::string serialize_import_names(const std::vector<ImportName> &names);
};

#endif //DSCRIPT_JSON_BUILDER_H