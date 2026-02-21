#include "resolver.h"

#include <sstream>

void Resolver::resolve(const std::vector<StmtPtr> &statements) {
    for (const StmtPtr &statement : statements) {
        resolve(statement);
    }
}

const std::vector<std::string> &Resolver::get_errors() const {
    return errors;
}

bool Resolver::had_error() const {
    return !errors.empty();
}

const std::unordered_map<const Expr *, int> &Resolver::get_locals() const {
    return locals;
}

Value Resolver::visit_block_stmt(BlockStmt *stmt) {
    begin_scope();
    for (const StmtPtr &statement : stmt->statements) {
        resolve(statement);
    }
    end_scope();
    return Value::none();
}

Value Resolver::visit_class_stmt(ClassStmt *stmt) {
    const ClassType enclosing_class = current_class;
    current_class = ClassType::Class;

    declare(stmt->name);
    define(stmt->name);

    if (stmt->super_class != nullptr) {
        if (const auto super_var = std::dynamic_pointer_cast<VariableExpr>(stmt->super_class);
            super_var != nullptr && super_var->name.literal.lexeme == stmt->name.literal.lexeme) {
            error_at(super_var->name, "A class cannot inherit from itself.");
        }

        current_class = ClassType::SubClass;
        resolve(stmt->super_class);

        begin_scope();
        scopes.back().insert_or_assign("super", true);
    }

    begin_scope();
    scopes.back().insert_or_assign("self", true);

    for (const auto &method : stmt->methods) {
        FunctionType declaration = FunctionType::Method;
        if (method->name.literal.lexeme == "init") {
            declaration = FunctionType::Initializer;
        }
        resolve_function(method->params, method->body, declaration);
    }

    end_scope();

    if (stmt->super_class != nullptr) {
        end_scope();
    }

    current_class = enclosing_class;
    return Value::none();
}

Value Resolver::visit_expression_stmt(ExpressionStmt *stmt) {
    resolve(stmt->expression);
    return Value::none();
}

Value Resolver::visit_function_stmt(FunctionStmt *stmt) {
    declare(stmt->name);
    define(stmt->name);
    resolve_function(stmt->params, stmt->body, FunctionType::Function);
    return Value::none();
}

Value Resolver::visit_if_stmt(IfStmt *stmt) {
    resolve(stmt->condition);
    resolve(stmt->thenBranch);
    resolve(stmt->elseBranch);
    return Value::none();
}

Value Resolver::visit_log_stmt(LogStmt *stmt) {
    resolve(stmt->expression);
    return Value::none();
}

Value Resolver::visit_return_stmt(ReturnStmt *stmt) {
    if (current_function == FunctionType::None) {
        error_at(stmt->keyword, "Cannot return from top-level code.");
    }

    if (stmt->value != nullptr) {
        if (current_function == FunctionType::Initializer) {
            error_at(stmt->keyword, "Cannot return a value from an initializer.");
        }
        resolve(stmt->value);
    }

    return Value::none();
}

Value Resolver::visit_let_stmt(LetStmt *stmt) {
    declare(stmt->name);
    resolve(stmt->init);
    define(stmt->name);
    return Value::none();
}

Value Resolver::visit_while_stmt(WhileStmt *stmt) {
    resolve(stmt->condition);
    resolve(stmt->body);
    return Value::none();
}

Value Resolver::visit_for_stmt(ForStmt *stmt) {
    begin_scope();
    resolve(stmt->init);
    resolve(stmt->condition);
    resolve(stmt->inc);
    resolve(stmt->body);
    end_scope();
    return Value::none();
}

Value Resolver::visit_def_stmt(DefStmt *stmt) {
    declare(stmt->new_keyword);
    define(stmt->new_keyword);
    resolve_function(stmt->params, stmt->body, FunctionType::Def);
    return Value::none();
}

Value Resolver::visit_assign_expr(AssignExpr *expr) {
    resolve(expr->value);
    resolve_local(expr, expr->name);
    return Value::none();
}

Value Resolver::visit_binary_expr(BinaryExpr *expr) {
    resolve(expr->left);
    resolve(expr->right);
    return Value::none();
}

Value Resolver::visit_call_expr(CallExpr *expr) {
    resolve(expr->callee);
    for (const ExprPtr &argument : expr->arguments) {
        resolve(argument);
    }
    return Value::none();
}

Value Resolver::visit_get_expr(GetExpr *expr) {
    resolve(expr->obj);
    return Value::none();
}

Value Resolver::visit_grouping_expr(GroupingExpr *expr) {
    resolve(expr->expression);
    return Value::none();
}

Value Resolver::visit_literal_expr(LiteralExpr *expr) {
    (void) expr;
    return Value::none();
}

Value Resolver::visit_logical_expr(LogicalExpr *expr) {
    resolve(expr->left);
    resolve(expr->right);
    return Value::none();
}

Value Resolver::visit_set_expr(SetExpr *expr) {
    resolve(expr->value);
    resolve(expr->obj);
    return Value::none();
}

Value Resolver::visit_super_expr(SuperExpr *expr) {
    if (current_class == ClassType::None) {
        error_at(expr->keyword, "Cannot use 'super' outside of a class.");
    } else if (current_class != ClassType::SubClass) {
        error_at(expr->keyword, "Cannot use 'super' in a class with no superclass.");
    }

    resolve_local(expr, expr->keyword);
    return Value::none();
}

Value Resolver::visit_self_expr(SelfExpr *expr) {
    if (current_class == ClassType::None) {
        error_at(expr->keyword, "Cannot use 'self' outside of a class.");
        return Value::none();
    }

    resolve_local(expr, expr->keyword);
    return Value::none();
}

Value Resolver::visit_unary_expr(UnaryExpr *expr) {
    resolve(expr->right);
    return Value::none();
}

Value Resolver::visit_variable_expr(VariableExpr *expr) {
    if (!scopes.empty()) {
        const auto it = scopes.back().find(expr->name.literal.lexeme);
        if (it != scopes.back().end() && !it->second) {
            error_at(expr->name, "Cannot read local variable in its own initializer.");
        }
    }

    resolve_local(expr, expr->name);
    return Value::none();
}

void Resolver::resolve(const StmtPtr &statement) {
    if (statement != nullptr) {
        statement->accept(this);
    }
}

void Resolver::resolve(const ExprPtr &expression) {
    if (expression != nullptr) {
        expression->accept(this);
    }
}

void Resolver::resolve_function(const std::vector<Token> &params, const std::vector<StmtPtr> &body, const FunctionType type) {
    const FunctionType enclosing_function = current_function;
    current_function = type;

    begin_scope();
    for (const Token &param : params) {
        declare(param);
        define(param);
    }

    for (const StmtPtr &statement : body) {
        resolve(statement);
    }

    end_scope();

    current_function = enclosing_function;
}

void Resolver::begin_scope() {
    scopes.emplace_back();
}

void Resolver::end_scope() {
    scopes.pop_back();
}

void Resolver::declare(const Token &name) {
    if (scopes.empty()) {
        return;
    }

    auto &scope = scopes.back();
    if (scope.find(name.literal.lexeme) != scope.end()) {
        error_at(name, "Variable with this name is already declared in this scope.");
    }

    scope.insert_or_assign(name.literal.lexeme, false);
}

void Resolver::define(const Token &name) {
    if (scopes.empty()) {
        return;
    }

    scopes.back().insert_or_assign(name.literal.lexeme, true);
}

void Resolver::resolve_local(const Expr *expr, const Token &name) {
    for (std::size_t depth = 0; depth < scopes.size(); ++depth) {
        const std::size_t idx = scopes.size() - 1 - depth;
        if (scopes[idx].find(name.literal.lexeme) != scopes[idx].end()) {
            locals.insert_or_assign(expr, static_cast<int>(depth));
            return;
        }
    }
}

void Resolver::error_at(const Token &token, const std::string &message) {
    std::ostringstream oss;
    oss << "[line " << token.file_pos.line_no << ", col " << token.file_pos.column_no << "] ResolverError: " << message;
    errors.push_back(oss.str());
}
