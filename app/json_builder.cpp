#include "json_builder.h"

#include <algorithm>
#include <sstream>

#include "../runtime/objs.h"

JsonAstBuilder::JsonAstBuilder(const std::unordered_map<const Expr *, int> &resolved_locals)
    : resolved_locals_(resolved_locals), next_expr_id_(0), current_expr_id_(0) {
}

std::string JsonAstBuilder::serialize_statements(const std::vector<StmtPtr> &statements) {
    return serialize_stmt_array(statements, *this);
}

std::string JsonAstBuilder::serialize_resolved_locals() const {
    std::vector<std::pair<std::size_t, int>> ordered;
    ordered.reserve(resolved_locals_.size());
    for (const auto &[expr, depth] : resolved_locals_) {
        const auto found = expr_ids_.find(expr);
        if (found == expr_ids_.end()) {
            continue;
        }
        ordered.emplace_back(found->second, depth);
    }

    std::sort(ordered.begin(), ordered.end(), [](const auto &left, const auto &right) {
        return left.first < right.first;
    });

    std::ostringstream oss;
    oss << "[";
    for (std::size_t i = 0; i < ordered.size(); ++i) {
        if (i > 0) {
            oss << ",";
        }
        oss << "{\"exprId\":" << ordered[i].first << ",\"depth\":" << ordered[i].second << "}";
    }
    oss << "]";
    return oss.str();
}

std::string JsonAstBuilder::json_escape(const std::string &value) {
    std::ostringstream oss;
    for (const char ch : value) {
        switch (ch) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << ch; break;
        }
    }
    return oss.str();
}

std::string JsonAstBuilder::serialize_token(const Token &token) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":" << static_cast<int>(token.type) << ",";
    oss << "\"lexeme\":\"" << json_escape(token.literal.lexeme) << "\",";
    oss << "\"line\":" << token.file_pos.line_no << ",";
    oss << "\"column\":" << token.file_pos.column_no;
    oss << "}";
    return oss.str();
}

std::string JsonAstBuilder::serialize_value(const Value &value) {
    std::ostringstream oss;
    switch (value.type) {
        case ValueType::None:
            return "{\"type\":\"None\"}";
        case ValueType::Boolean:
            return value.as.boolean ? "{\"type\":\"Boolean\",\"value\":true}" : "{\"type\":\"Boolean\",\"value\":false}";
        case ValueType::Number:
            oss << "{\"type\":\"Number\",\"value\":" << value.as.number << "}";
            return oss.str();
        case ValueType::Object:
            break;
    }

    if (value.as.object == nullptr) {
        return "{\"type\":\"Object\",\"objectType\":\"null\"}";
    }

    if (value.as.object->get_type() == ObjType::String) {
        const auto str = std::static_pointer_cast<ObjString>(value.as.object);
        return "{\"type\":\"String\",\"value\":\"" + json_escape(str->chars) + "\"}";
    }

    return "{\"type\":\"Object\",\"objectType\":" + std::to_string(static_cast<int>(value.as.object->get_type())) + "}";
}

std::size_t JsonAstBuilder::ensure_expr_id(const Expr *expr) {
    const auto [it, inserted] = expr_ids_.emplace(expr, next_expr_id_);
    if (inserted) {
        ++next_expr_id_;
    }
    return it->second;
}

std::string JsonAstBuilder::serialize_stmt(const StmtPtr &stmt) {
    if (stmt == nullptr) {
        return "null";
    }
    stmt->accept(this);
    return current_json_;
}

std::string JsonAstBuilder::serialize_expr(const ExprPtr &expr) {
    if (expr == nullptr) {
        return "null";
    }
    current_expr_id_ = ensure_expr_id(expr.get());
    expr->accept(this);
    return current_json_;
}

std::string JsonAstBuilder::serialize_stmt_array(const std::vector<StmtPtr> &stmts, JsonAstBuilder &builder) {
    std::ostringstream oss;
    oss << "[";
    for (std::size_t i = 0; i < stmts.size(); ++i) {
        if (i > 0) {
            oss << ",";
        }
        oss << builder.serialize_stmt(stmts[i]);
    }
    oss << "]";
    return oss.str();
}

std::string JsonAstBuilder::serialize_expr_array(const std::vector<ExprPtr> &exprs, JsonAstBuilder &builder) {
    std::ostringstream oss;
    oss << "[";
    for (std::size_t i = 0; i < exprs.size(); ++i) {
        if (i > 0) {
            oss << ",";
        }
        oss << builder.serialize_expr(exprs[i]);
    }
    oss << "]";
    return oss.str();
}

std::string JsonAstBuilder::serialize_params(const std::vector<FunctionStmt::Parameter> &params) {
    std::ostringstream params_json;
    params_json << "[";
    for (std::size_t i = 0; i < params.size(); ++i) {
        if (i > 0) {
            params_json << ",";
        }
        params_json << "{\"name\":" << serialize_token(params[i].name)
                    << ",\"isVariadic\":" << (params[i].is_variadic ? "true" : "false") << "}";
    }
    params_json << "]";
    return params_json.str();
}

std::string JsonAstBuilder::serialize_block_literals(const std::optional<std::vector<BlockLiteral>> &blocks) {
    if (!blocks.has_value()) {
        return "null";
    }

    std::ostringstream blocks_json;
    blocks_json << "[";
    for (std::size_t i = 0; i < blocks->size(); ++i) {
        if (i > 0) {
            blocks_json << ",";
        }
        blocks_json << "{\"name\":" << serialize_token((*blocks)[i].name) << ",\"expect\":";
        if ((*blocks)[i].expect.has_value()) {
            blocks_json << serialize_token((*blocks)[i].expect.value());
        } else {
            blocks_json << "null";
        }
        blocks_json << "}";
    }
    blocks_json << "]";
    return blocks_json.str();
}

std::string JsonAstBuilder::serialize_import_names(const std::vector<ImportName> &names) {
    std::ostringstream names_json;
    names_json << "[";
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i > 0) {
            names_json << ",";
        }
        names_json << "{\"name\":" << serialize_token(names[i].name)
                   << ",\"alias\":" << serialize_token(names[i].alias) << "}";
    }
    names_json << "]";
    return names_json.str();
}

std::string JsonAstBuilder::serialize_class_fields(const std::vector<ClassField> &fields, JsonAstBuilder &builder) {
    std::ostringstream fields_json;
    fields_json << "[";
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) {
            fields_json << ",";
        }

        fields_json << "{\"name\":" << serialize_token(fields[i].name)
                    << ",\"init\":" << builder.serialize_expr(fields[i].init)
                    << "}";
    }
    fields_json << "]";
    return fields_json.str();
}

Value JsonAstBuilder::visit_block_stmt(BlockStmt *stmt) {
    current_json_ = "{\"type\":\"BlockStmt\",\"statements\":" + serialize_stmt_array(stmt->statements, *this) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_class_stmt(ClassStmt *stmt) {
    std::ostringstream methods;
    methods << "[";
    for (std::size_t i = 0; i < stmt->methods.size(); ++i) {
        if (i > 0) {
            methods << ",";
        }
        methods << serialize_stmt(stmt->methods[i]);
    }
    methods << "]";

    current_json_ = "{\"type\":\"ClassStmt\",\"name\":" + serialize_token(stmt->name) +
                    ",\"super\":" + serialize_expr(stmt->super_class) +
                    ",\"methods\":" + methods.str() +
                    ",\"private_fields\":" + serialize_class_fields(stmt->private_fields, *this) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_expression_stmt(ExpressionStmt *stmt) {
    current_json_ = "{\"type\":\"ExpressionStmt\",\"expression\":" + serialize_expr(stmt->expression) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_function_stmt(FunctionStmt *stmt) {
    current_json_ = "{\"type\":\"FunctionStmt\",\"name\":" + serialize_token(stmt->name) +
                    ",\"params\":" + serialize_params(stmt->params) +
                    ",\"blocks\":" + serialize_block_literals(stmt->blocks) +
                    ",\"body\":" + serialize_stmt_array(stmt->body, *this) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_if_stmt(IfStmt *stmt) {
    current_json_ = "{\"type\":\"IfStmt\",\"condition\":" + serialize_expr(stmt->condition) +
                    ",\"then\":" + serialize_stmt(stmt->thenBranch) +
                    ",\"else\":" + serialize_stmt(stmt->elseBranch) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_log_stmt(LogStmt *stmt) {
    current_json_ = "{\"type\":\"LogStmt\",\"expression\":" + serialize_expr(stmt->expression) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_return_stmt(ReturnStmt *stmt) {
    current_json_ = "{\"type\":\"ReturnStmt\",\"keyword\":" + serialize_token(stmt->keyword) +
                    ",\"values\":" + serialize_expr_array(stmt->values, *this) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_let_stmt(LetStmt *stmt) {
    std::ostringstream names;
    names << "[";
    for (std::size_t i = 0; i < stmt->names.size(); ++i) {
        if (i > 0) {
            names << ",";
        }
        names << serialize_token(stmt->names[i]);
    }
    names << "]";

    current_json_ = "{\"type\":\"LetStmt\",\"keyword\":" + serialize_token(stmt->keyword) +
                    ",\"names\":" + names.str() +
                    ",\"inits\":" + serialize_expr_array(stmt->inits, *this) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_while_stmt(WhileStmt *stmt) {
    current_json_ = "{\"type\":\"WhileStmt\",\"condition\":" + serialize_expr(stmt->condition) +
                    ",\"body\":" + serialize_stmt(stmt->body) +
                    ",\"finally\":" + serialize_stmt(stmt->finally_clause) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_for_stmt(ForStmt *stmt) {
    current_json_ = "{\"type\":\"ForStmt\",\"init\":" + serialize_stmt(stmt->init) +
                    ",\"condition\":" + serialize_expr(stmt->condition) +
                    ",\"inc\":" + serialize_expr(stmt->inc) +
                    ",\"body\":" + serialize_stmt(stmt->body) +
                    ",\"finally\":" + serialize_stmt(stmt->finally_clause) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_do_while_stmt(DoWhileStmt *stmt) {
    current_json_ = "{\"type\":\"DoWhileStmt\",\"body\":" + serialize_stmt(stmt->body) +
                    ",\"condition\":" + serialize_expr(stmt->condition) +
                    ",\"finally\":" + serialize_stmt(stmt->finally_clause) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_break_stmt(BreakStmt *stmt) {
    current_json_ = "{\"type\":\"BreakStmt\",\"keyword\":" + serialize_token(stmt->keyword) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_continue_stmt(ContinueStmt *stmt) {
    current_json_ = "{\"type\":\"ContinueStmt\",\"keyword\":" + serialize_token(stmt->keyword) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_def_stmt(DefStmt *stmt) {
    current_json_ = "{\"type\":\"DefStmt\",\"name\":" + serialize_token(stmt->new_keyword) +
                    ",\"params\":" + serialize_params(stmt->params) +
                    ",\"blocks\":" + serialize_block_literals(stmt->blocks) +
                    ",\"body\":" + serialize_stmt_array(stmt->body, *this) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_import_stmt(ImportStmt *stmt) {
    current_json_ = "{\"type\":\"ImportStmt\",\"keyword\":" + serialize_token(stmt->keyword) +
                    ",\"path\":" + serialize_token(stmt->path) +
                    ",\"alias\":" + serialize_token(stmt->alias) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_from_import_stmt(FromImportStmt *stmt) {
    current_json_ = "{\"type\":\"FromImportStmt\",\"keyword\":" + serialize_token(stmt->keyword) +
                    ",\"path\":" + serialize_token(stmt->path) +
                    ",\"names\":" + serialize_import_names(stmt->names) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_export_stmt(ExportStmt *stmt) {
    current_json_ = "{\"type\":\"ExportStmt\",\"declaration\":" + serialize_stmt(stmt->declaration) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_assign_expr(AssignExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"AssignExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"name\":" + serialize_token(expr->name) +
                    ",\"value\":" + serialize_expr(expr->value) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_binary_expr(BinaryExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"BinaryExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"left\":" + serialize_expr(expr->left) +
                    ",\"op\":" + serialize_token(expr->op) +
                    ",\"right\":" + serialize_expr(expr->right) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_call_expr(CallExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"CallExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"callee\":" + serialize_expr(expr->callee) +
                    ",\"paren\":" + serialize_token(expr->paren) +
                    ",\"arguments\":" + serialize_expr_array(expr->arguments, *this) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_function_expr(FunctionExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"FunctionExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"body\":" + serialize_stmt_array(expr->body, *this) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_index_expr(IndexExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"IndexExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"obj\":" + serialize_expr(expr->obj) +
                    ",\"bracket\":" + serialize_token(expr->bracket) +
                    ",\"key\":" + serialize_expr(expr->key) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_list_expr(ListExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"ListExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"elements\":" + serialize_expr_array(expr->elements, *this) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_named_block_expr(NamedBlockExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"NamedBlockExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"name\":" + serialize_token(expr->name) +
                    ",\"value\":" + serialize_expr(expr->value) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_get_expr(GetExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"GetExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"obj\":" + serialize_expr(expr->obj) +
                    ",\"name\":" + serialize_token(expr->name) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_grouping_expr(GroupingExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"GroupingExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"expression\":" + serialize_expr(expr->expression) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_literal_expr(LiteralExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"LiteralExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"value\":" + serialize_value(expr->value) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_logical_expr(LogicalExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"LogicalExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"left\":" + serialize_expr(expr->left) +
                    ",\"op\":" + serialize_token(expr->op) +
                    ",\"right\":" + serialize_expr(expr->right) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_set_expr(SetExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"SetExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"obj\":" + serialize_expr(expr->obj) +
                    ",\"name\":" + serialize_token(expr->name) +
                    ",\"value\":" + serialize_expr(expr->value) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_set_index_expr(SetIndexExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"SetIndexExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"obj\":" + serialize_expr(expr->obj) +
                    ",\"bracket\":" + serialize_token(expr->bracket) +
                    ",\"key\":" + serialize_expr(expr->key) +
                    ",\"value\":" + serialize_expr(expr->value) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_super_expr(SuperExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"SuperExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"keyword\":" + serialize_token(expr->keyword) +
                    ",\"method\":" + serialize_token(expr->method) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_self_expr(SelfExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"SelfExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"keyword\":" + serialize_token(expr->keyword) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_unary_expr(UnaryExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"UnaryExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"op\":" + serialize_token(expr->op) +
                    ",\"right\":" + serialize_expr(expr->right) + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_update_expr(UpdateExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"UpdateExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"target\":" + serialize_expr(expr->target) +
                    ",\"op\":" + serialize_token(expr->op) +
                    ",\"isPrefix\":" + (expr->is_prefix ? "true" : "false") + "}";
    return Value::none();
}

Value JsonAstBuilder::visit_variable_expr(VariableExpr *expr) {
    const std::size_t expr_id = current_expr_id_;
    current_json_ = "{\"type\":\"VariableExpr\",\"id\":" + std::to_string(expr_id) +
                    ",\"name\":" + serialize_token(expr->name) + "}";
    return Value::none();
}