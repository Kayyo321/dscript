//
// Created by sullivanb on 2/21/26.
//

#ifndef DSCRIPT_VM_H
#define DSCRIPT_VM_H

#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

#include "env.h"
#include "objs.h"
#include "../parsing/a_visitor.h"
#include "../parsing/expr.h"
#include "../parsing/stmt.h"

class RuntimeError;

class Vm : public AVisitor {
public:
	Vm();

	void interpret(const std::vector<StmtPtr> &statements);
	void execute_block(const std::vector<StmtPtr> &statements, const std::shared_ptr<Environment> &environment);
	void set_locals(const std::unordered_map<const Expr *, int> &resolved_locals);
	void set_source(std::string path);
	void register_precompiled_module(
		std::string module_id,
		std::vector<StmtPtr> statements,
		std::unordered_map<const Expr *, int> resolved_locals,
		std::unordered_map<std::string, std::string> import_map,
		std::string source_path_hint
	);
	void clear_precompiled_modules();
	bool invoke_main_if_present();

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
	Value evaluate(const ExprPtr &expr);
	void execute(const StmtPtr &stmt);
	std::shared_ptr<ObjModule> load_module(const std::string &raw_path, const FilePos &location);
	std::shared_ptr<ObjModule> load_stdlib_module(const std::string &name, const FilePos &location);
	std::optional<std::string> resolve_precompiled_import_id(const std::string &raw_path) const;
	std::string resolve_module_path(const std::string &raw_path) const;
	std::vector<std::string> read_module_lines(const std::string &path) const;
	Value lookup_variable(const Token &name, const Expr *expr) const;
	std::string format_runtime_error(const RuntimeError &error) const;

	static bool is_truthy(const Value &value);
	static bool is_equal(const Value &left, const Value &right);
	static bool is_type_match(const Value &left, const Value &right);
	static void assert_number_operand(const Token &op, const Value &value);
	static void assert_number_operands(const Token &op, const Value &left, const Value &right);
	static std::string stringify(const Value &value);

	std::shared_ptr<Environment> globals;
	std::shared_ptr<Environment> environment;
	std::unordered_map<const Expr *, int> locals;
	std::string source_path;
	std::vector<std::string> source_lines;
	std::unordered_map<std::string, std::vector<std::string>> source_lines_by_path;
	std::unordered_map<std::string, std::shared_ptr<ObjModule>> module_cache;
	std::set<std::string> loading_modules;
	struct PrecompiledModule {
		std::vector<StmtPtr> statements;
		std::unordered_map<const Expr *, int> resolved_locals;
		std::unordered_map<std::string, std::string> import_map;
		std::string source_path_hint;
	};
	std::unordered_map<std::string, PrecompiledModule> precompiled_modules;
	std::vector<std::string> precompiled_module_stack;
	std::unordered_map<std::string, Value> *active_module_exports{nullptr};
};

#endif //DSCRIPT_VM_H