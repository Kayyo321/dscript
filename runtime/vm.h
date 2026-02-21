//
// Created by sullivanb on 2/21/26.
//

#ifndef DSCRIPT_VM_H
#define DSCRIPT_VM_H

#include <memory>
#include <unordered_map>
#include <vector>

#include "env.h"
#include "objs.h"
#include "../parsing/a_visitor.h"
#include "../parsing/expr.h"
#include "../parsing/stmt.h"

class Vm : public AVisitor {
public:
	Vm();

	void interpret(const std::vector<StmtPtr> &statements);
	void execute_block(const std::vector<StmtPtr> &statements, const std::shared_ptr<Environment> &environment);
	void set_locals(const std::unordered_map<const Expr *, int> &resolved_locals);
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
	Value visit_def_stmt(DefStmt *stmt) override;

	Value visit_assign_expr(AssignExpr *expr) override;
	Value visit_binary_expr(BinaryExpr *expr) override;
	Value visit_call_expr(CallExpr *expr) override;
	Value visit_get_expr(GetExpr *expr) override;
	Value visit_grouping_expr(GroupingExpr *expr) override;
	Value visit_literal_expr(LiteralExpr *expr) override;
	Value visit_logical_expr(LogicalExpr *expr) override;
	Value visit_set_expr(SetExpr *expr) override;
	Value visit_super_expr(SuperExpr *expr) override;
	Value visit_self_expr(SelfExpr *expr) override;
	Value visit_unary_expr(UnaryExpr *expr) override;
	Value visit_variable_expr(VariableExpr *expr) override;

private:
	Value evaluate(const ExprPtr &expr);
	void execute(const StmtPtr &stmt);
	Value lookup_variable(const Token &name, const Expr *expr) const;

	static bool is_truthy(const Value &value);
	static bool is_equal(const Value &left, const Value &right);
	static void assert_number_operand(const Token &op, const Value &value);
	static void assert_number_operands(const Token &op, const Value &left, const Value &right);
	static std::string stringify(const Value &value);

	std::shared_ptr<Environment> globals;
	std::shared_ptr<Environment> environment;
	std::unordered_map<const Expr *, int> locals;
};

#endif //DSCRIPT_VM_H