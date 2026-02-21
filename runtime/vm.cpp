#include "vm.h"

#include <cmath>
#include <iostream>
#include <sstream>

#include "throwables.h"

Vm::Vm()
	: globals(std::make_shared<Environment>()), environment(globals) {}

void Vm::interpret(const std::vector<StmtPtr> &statements) {
	try {
		for (const auto &statement : statements) {
			execute(statement);
		}
	} catch (const RuntimeError &error) {
		std::cerr << "RuntimeError: " << error.what() << '\n';
	}
}

void Vm::execute_block(const std::vector<StmtPtr> &statements, const std::shared_ptr<Environment> &new_environment) {
	const auto previous = environment;

	try {
		environment = new_environment;
		for (const auto &statement : statements) {
			execute(statement);
		}
	} catch (...) {
		environment = previous;
		throw;
	}

	environment = previous;
}

void Vm::set_locals(const std::unordered_map<const Expr *, int> &resolved_locals) {
	locals = resolved_locals;
}

bool Vm::invoke_main_if_present() {
	try {
		const Value main = globals->get("main");
		if (main.type != ValueType::Object) {
			throw RuntimeError("'main' exists but is not callable.");
		}

		const auto callable = std::dynamic_pointer_cast<Callable>(main.as.object);
		if (callable == nullptr) {
			throw RuntimeError("'main' exists but is not callable.");
		}

		if (callable->arity() != 0) {
			throw RuntimeError("'main' must take 0 arguments.");
		}

		callable->call(this, {});
		return true;
	} catch (const RuntimeError &error) {
		const std::string message = error.what();
		if (message.rfind("Undefined variable 'main'", 0) == 0) {
			return false;
		}

		std::cerr << "RuntimeError: " << error.what() << '\n';
		return false;
	}
}

Value Vm::visit_block_stmt(BlockStmt *stmt) {
	execute_block(stmt->statements, std::make_shared<Environment>(environment));
	return Value::none();
}

Value Vm::visit_class_stmt(ClassStmt *stmt) {
	Value super_class = Value::none();
	std::shared_ptr<ObjClass> super_class_obj = nullptr;

	if (stmt->super_class != nullptr) {
		super_class = evaluate(stmt->super_class);
		if (super_class.type != ValueType::Object) {
			throw RuntimeError("Superclass must be a class.");
		}

		super_class_obj = std::dynamic_pointer_cast<ObjClass>(super_class.as.object);
		if (super_class_obj == nullptr) {
			throw RuntimeError("Superclass must be a class.");
		}
	}

	environment->define(stmt->name.literal.lexeme, Value::none());

	if (super_class_obj != nullptr) {
		environment = std::make_shared<Environment>(environment);
		environment->define("super", super_class);
	}

	std::map<std::string, std::shared_ptr<ObjFunction>> methods;
	for (const auto &method : stmt->methods) {
		const bool is_init = method->name.literal.lexeme == "init";
		methods.insert_or_assign(method->name.literal.lexeme, std::make_shared<ObjFunction>(method, environment, is_init));
	}

	const auto klass = std::make_shared<ObjClass>(stmt->name.literal.lexeme, methods);

	if (super_class_obj != nullptr) {
		environment = environment->ancestor(1);
	}

	environment->assign(stmt->name.literal.lexeme, Value::object(klass));
	return Value::none();
}

Value Vm::visit_expression_stmt(ExpressionStmt *stmt) {
	(void) evaluate(stmt->expression);
	return Value::none();
}

Value Vm::visit_function_stmt(FunctionStmt *stmt) {
	auto declaration = std::make_shared<FunctionStmt>(*stmt);
	environment->define(stmt->name.literal.lexeme, Value::object(ObjFunction::basic(declaration, environment)));
	return Value::none();
}

Value Vm::visit_if_stmt(IfStmt *stmt) {
	if (is_truthy(evaluate(stmt->condition))) {
		execute(stmt->thenBranch);
	} else if (stmt->elseBranch != nullptr) {
		execute(stmt->elseBranch);
	}

	return Value::none();
}

Value Vm::visit_log_stmt(LogStmt *stmt) {
	const Value value = evaluate(stmt->expression);
	value.print(std::cout);
	std::cout << '\n';
	return Value::none();
}

Value Vm::visit_return_stmt(ReturnStmt *stmt) {
	Value value = Value::none();
	if (stmt->value != nullptr) {
		value = evaluate(stmt->value);
	}

	throw Return(value);
}

Value Vm::visit_let_stmt(LetStmt *stmt) {
	Value value = Value::none();
	if (stmt->init != nullptr) {
		value = evaluate(stmt->init);
	}

	environment->define(stmt->name.literal.lexeme, value);
	return Value::none();
}

Value Vm::visit_while_stmt(WhileStmt *stmt) {
	while (is_truthy(evaluate(stmt->condition))) {
		execute(stmt->body);
	}

	return Value::none();
}

Value Vm::visit_for_stmt(ForStmt *stmt) {
	if (stmt->init != nullptr) {
		execute(stmt->init);
	}

	while (stmt->condition == nullptr || is_truthy(evaluate(stmt->condition))) {
		execute(stmt->body);
		if (stmt->inc != nullptr) {
			(void) evaluate(stmt->inc);
		}
	}

	return Value::none();
}

Value Vm::visit_def_stmt(DefStmt *stmt) {
	auto declaration = std::make_shared<FunctionStmt>(stmt->new_keyword, stmt->params, stmt->body, false);
	if (stmt->blocks.has_value()) {
		declaration = std::make_shared<FunctionStmt>(stmt->new_keyword, stmt->params, stmt->blocks.value(), stmt->body, false);
	}

	environment->define(stmt->new_keyword.literal.lexeme, Value::object(ObjFunction::basic(declaration, environment)));
	return Value::none();
}

Value Vm::visit_assign_expr(AssignExpr *expr) {
	const Value value = evaluate(expr->value);

	if (const auto it = locals.find(expr); it != locals.end()) {
		environment->assign_at(it->second, expr->name.literal.lexeme, value);
	} else {
		globals->assign(expr->name.literal.lexeme, value);
	}

	return value;
}

Value Vm::visit_binary_expr(BinaryExpr *expr) {
	const Value left = evaluate(expr->left);
	const Value right = evaluate(expr->right);

	switch (expr->op.type) {
		case TokenType::Plus:
			if (left.type == ValueType::Number && right.type == ValueType::Number) {
				return Value::number(left.as.number + right.as.number);
			}

			if (left.type == ValueType::Object && right.type == ValueType::Object &&
				left.as.object->get_type() == ObjType::String && right.as.object->get_type() == ObjType::String) {
				const auto lhs = std::static_pointer_cast<ObjString>(left.as.object);
				const auto rhs = std::static_pointer_cast<ObjString>(right.as.object);
				return Value::object(std::make_shared<ObjString>(lhs->chars + rhs->chars));
			}

			throw RuntimeError("Operands to '+' must both be numbers or strings.");

		case TokenType::Minus:
			assert_number_operands(expr->op, left, right);
			return Value::number(left.as.number - right.as.number);

		case TokenType::Star:
			assert_number_operands(expr->op, left, right);
			return Value::number(left.as.number * right.as.number);

		case TokenType::Slash:
			assert_number_operands(expr->op, left, right);
			return Value::number(left.as.number / right.as.number);

		case TokenType::Modulo:
			assert_number_operands(expr->op, left, right);
			return Value::number(std::fmod(left.as.number, right.as.number));

		case TokenType::Power:
			assert_number_operands(expr->op, left, right);
			return Value::number(std::pow(left.as.number, right.as.number));

		case TokenType::GreaterThan:
			assert_number_operands(expr->op, left, right);
			return Value::boolean(left.as.number > right.as.number);

		case TokenType::GreaterEqual:
			assert_number_operands(expr->op, left, right);
			return Value::boolean(left.as.number >= right.as.number);

		case TokenType::LessThan:
			assert_number_operands(expr->op, left, right);
			return Value::boolean(left.as.number < right.as.number);

		case TokenType::LessEqual:
			assert_number_operands(expr->op, left, right);
			return Value::boolean(left.as.number <= right.as.number);

		case TokenType::Is:
			return Value::boolean(is_equal(left, right));

		case TokenType::Not:
			return Value::boolean(!is_equal(left, right));

		default:
			break;
	}

	return Value::none();
}

Value Vm::visit_call_expr(CallExpr *expr) {
	const Value callee = evaluate(expr->callee);

	std::vector<Value> args;
	args.reserve(expr->arguments.size());
	for (const auto &argument : expr->arguments) {
		args.push_back(evaluate(argument));
	}

	if (callee.type != ValueType::Object) {
		throw RuntimeError("Can only call callable objects.");
	}

	const auto callable = std::dynamic_pointer_cast<Callable>(callee.as.object);
	if (callable == nullptr) {
		throw RuntimeError("Can only call callable objects.");
	}

	if (static_cast<int>(args.size()) != callable->arity()) {
		throw RuntimeError("Argument count mismatch.");
	}

	return callable->call(this, args);
}

Value Vm::visit_function_expr(FunctionExpr *expr) {
	const Token literal_name{TokenType::Identifier, "<literal>", FilePos{}};
	auto declaration = std::make_shared<FunctionStmt>(literal_name, std::vector<Token>{}, expr->body, false);
	return Value::object(ObjFunction::basic(declaration, environment));
}

Value Vm::visit_get_expr(GetExpr *expr) {
	const Value obj = evaluate(expr->obj);
	if (obj.type != ValueType::Object) {
		throw RuntimeError("Only instances have properties.");
	}

	const auto instance = std::dynamic_pointer_cast<ObjInstance>(obj.as.object);
	if (instance == nullptr) {
		throw RuntimeError("Only instances have properties.");
	}

	return instance->get(expr->name.literal.lexeme);
}

Value Vm::visit_grouping_expr(GroupingExpr *expr) {
	return evaluate(expr->expression);
}

Value Vm::visit_literal_expr(LiteralExpr *expr) {
	return expr->value;
}

Value Vm::visit_logical_expr(LogicalExpr *expr) {
	const Value left = evaluate(expr->left);

	if (expr->op.type == TokenType::And) {
		if (!is_truthy(left)) {
			return left;
		}
	} else {
		if (is_truthy(left)) {
			return left;
		}
	}

	return evaluate(expr->right);
}

Value Vm::visit_set_expr(SetExpr *expr) {
	const Value obj = evaluate(expr->obj);
	if (obj.type != ValueType::Object) {
		throw RuntimeError("Only instances have fields.");
	}

	const auto instance = std::dynamic_pointer_cast<ObjInstance>(obj.as.object);
	if (instance == nullptr) {
		throw RuntimeError("Only instances have fields.");
	}

	const Value value = evaluate(expr->value);
	instance->set(expr->name.literal.lexeme, value);
	return value;
}

Value Vm::visit_super_expr(SuperExpr *expr) {
	const auto super_it = locals.find(expr);
	if (super_it == locals.end()) {
		throw RuntimeError("Unable to resolve 'super'.");
	}

	const int distance = super_it->second;
	Value super = environment->get_at(distance, "super");
	Value self = environment->get_at(distance - 1, "self");

	const auto super_class = std::dynamic_pointer_cast<ObjClass>(super.as.object);
	const auto instance = std::dynamic_pointer_cast<ObjInstance>(self.as.object);

	const auto method = super_class->find_method(expr->method.literal.lexeme);
	if (method == nullptr) {
		throw RuntimeError("Undefined property '" + expr->method.literal.lexeme + "'.");
	}

	return Value::object(method->bind(instance.get()));
}

Value Vm::visit_self_expr(SelfExpr *expr) {
	return lookup_variable(expr->keyword, expr);
}

Value Vm::visit_unary_expr(UnaryExpr *expr) {
	const Value right = evaluate(expr->right);

	switch (expr->op.type) {
		case TokenType::Minus:
			assert_number_operand(expr->op, right);
			return Value::number(-right.as.number);

		case TokenType::Not:
			return Value::boolean(!is_truthy(right));

		default:
			break;
	}

	return Value::none();
}

Value Vm::visit_variable_expr(VariableExpr *expr) {
	return lookup_variable(expr->name, expr);
}

Value Vm::evaluate(const ExprPtr &expr) {
	return expr->accept(this);
}

void Vm::execute(const StmtPtr &stmt) {
	if (stmt != nullptr) {
		(void) stmt->accept(this);
	}
}

Value Vm::lookup_variable(const Token &name, const Expr *expr) const {
	const auto local = locals.find(expr);
	if (local != locals.end()) {
		return environment->get_at(local->second, name.literal.lexeme);
	}

	return globals->get(name.literal.lexeme);
}

bool Vm::is_truthy(const Value &value) {
	switch (value.type) {
		case ValueType::None:
			return false;
		case ValueType::Boolean:
			return value.as.boolean;
		default:
			return true;
	}
}

bool Vm::is_equal(const Value &left, const Value &right) {
	return left == right;
}

void Vm::assert_number_operand(const Token &op, const Value &value) {
	(void) op;
	if (value.type != ValueType::Number) {
		throw RuntimeError("Operand must be a number.");
	}
}

void Vm::assert_number_operands(const Token &op, const Value &left, const Value &right) {
	(void) op;
	if (left.type != ValueType::Number || right.type != ValueType::Number) {
		throw RuntimeError("Operands must be numbers.");
	}
}

std::string Vm::stringify(const Value &value) {
	std::ostringstream oss;
	value.print(oss);
	return oss.str();
}
