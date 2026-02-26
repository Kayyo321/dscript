#include "vm.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

#include "../lexing/lexer.h"
#include "../parsing/parser.h"
#include "../resolving/resolver.h"
#include "stdlibs/registry.h"
#include "throwables.h"

static const char *kind_to_string(const RuntimeErrorKind kind) {
	switch (kind) {
		case RuntimeErrorKind::Type:
			return "TypeError";
		case RuntimeErrorKind::Name:
			return "NameError";
		case RuntimeErrorKind::Call:
			return "CallError";
		case RuntimeErrorKind::Arity:
			return "ArityError";
		case RuntimeErrorKind::Property:
			return "PropertyError";
		case RuntimeErrorKind::Index:
			return "IndexError";
		default:
			return "RuntimeError";
	}
}

static std::size_t compute_highlight_length(const std::string &line, const std::size_t col_no) {
	if (line.empty()) {
		return 1;
	}

	if (col_no == 0) {
		return 1;
	}

	const std::size_t start = col_no - 1;
	if (start >= line.size()) {
		return 1;
	}

	const char c = line[start];

	auto is_ident_char = [](const char ch) {
		return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '$' || ch == '%';
	};

	auto is_operator_char = [](const char ch) {
		switch (ch) {
			case '+':
			case '-':
			case '*':
			case '/':
			case '%':
			case '^':
			case '=':
			case '!':
			case '<':
			case '>':
			case '&':
			case '|':
			case '.':
				return true;
			default:
				return false;
		}
	};

	std::size_t end = start;

	if (c == '\'' || c == '"') {
		const char quote = c;
		++end;
		while (end < line.size()) {
			if (line[end] == '\\' && end + 1 < line.size()) {
				end += 2;
				continue;
			}

			if (line[end] == quote) {
				++end;
				break;
			}

			++end;
		}
	} else if (is_ident_char(c)) {
		while (end < line.size() && is_ident_char(line[end])) {
			++end;
		}

		while (end < line.size()) {
			if (line[end] == '.') {
				std::size_t i = end + 1;
				if (i < line.size() && is_ident_char(line[i])) {
					while (i < line.size() && is_ident_char(line[i])) {
						++i;
					}
					end = i;
					continue;
				}
			}

			if (line[end] == '[') {
				std::size_t i = end;
				int depth = 0;
				while (i < line.size()) {
					if (line[i] == '[') {
						++depth;
					} else if (line[i] == ']') {
						--depth;
						if (depth == 0) {
							++i;
							break;
						}
					}
					++i;
				}
				end = i;
				continue;
			}

			break;
		}
	} else if (is_operator_char(c)) {
		while (end < line.size() && is_operator_char(line[end])) {
			++end;
		}
	} else if (c == '[') {
		std::size_t i = end;
		int depth = 0;
		while (i < line.size()) {
			if (line[i] == '[') {
				++depth;
			} else if (line[i] == ']') {
				--depth;
				if (depth == 0) {
					++i;
					break;
				}
			}
			++i;
		}
		end = i;
	} else {
		++end;
	}

	std::size_t length = end > start ? (end - start) : 1;
	if (length > 80) {
		length = 80;
	}

	return length;
}

Vm::Vm()
	: globals(std::make_shared<Environment>()), environment(globals) {
	const auto &registry = get_stdlib_registry();
	if (const auto core_it = registry.find("core"); core_it != registry.end()) {
		std::unordered_map<std::string, Value> core_exports = core_it->second();
		for (const auto &[name, value] : core_exports) {
			globals->define(name, value);
		}
	}
}

void Vm::interpret(const std::vector<StmtPtr> &statements) {
	try {
		for (const auto &statement : statements) {
			execute(statement);
		}
	} catch (const RuntimeError &error) {
		std::cerr << format_runtime_error(error) << '\n';
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

void Vm::set_source(std::string path) {
	source_path = std::move(path);
	source_lines.clear();

	if (!source_path.empty()) {
		std::ifstream in(source_path);
		std::string line;
		while (std::getline(in, line)) {
			source_lines.push_back(line);
		}
		source_lines_by_path.insert_or_assign(source_path, source_lines);
	}
}

void Vm::register_precompiled_module(
	std::string module_id,
	std::vector<StmtPtr> statements,
	std::unordered_map<const Expr *, int> resolved_locals,
	std::unordered_map<std::string, std::string> import_map,
	std::string source_path_hint
) {
	precompiled_modules.insert_or_assign(module_id, PrecompiledModule{
		std::move(statements),
		std::move(resolved_locals),
		std::move(import_map),
		std::move(source_path_hint),
	});
}

void Vm::clear_precompiled_modules() {
	precompiled_modules.clear();
	precompiled_module_stack.clear();
}

std::optional<std::string> Vm::resolve_precompiled_import_id(const std::string &raw_path) const {
	if (const auto direct = precompiled_modules.find(raw_path); direct != precompiled_modules.end()) {
		return direct->first;
	}

	if (!precompiled_module_stack.empty()) {
		const std::string &current_module_id = precompiled_module_stack.back();
		if (const auto current = precompiled_modules.find(current_module_id); current != precompiled_modules.end()) {
			if (const auto mapped = current->second.import_map.find(raw_path); mapped != current->second.import_map.end()) {
				return mapped->second;
			}
		}
	}

	return std::nullopt;
}

std::string Vm::resolve_module_path(const std::string &raw_path) const {
	namespace fs = std::filesystem;
	fs::path import_path(raw_path);

	if (import_path.is_absolute()) {
		return fs::weakly_canonical(import_path).string();
	}

	fs::path base_dir = source_path.empty() ? fs::current_path() : fs::path(source_path).parent_path();
	return fs::weakly_canonical(base_dir / import_path).string();
}

std::vector<std::string> Vm::read_module_lines(const std::string &path) const {
	std::ifstream in(path);
	if (!in.is_open()) {
		throw NameError("Could not open module '" + path + "'.");
	}

	std::vector<std::string> lines;
	std::string line;
	while (std::getline(in, line)) {
		lines.push_back(line);
	}

	return lines;
}

std::shared_ptr<ObjModule> Vm::load_stdlib_module(const std::string &name, const FilePos &location) {
	const auto &registry = get_stdlib_registry();
	const auto factory_it = registry.find(name);
	if (factory_it == registry.end()) {
		throw NameError("Unknown stdlib module '" + name + "'.", location);
	}

	const std::string cache_key = "std:" + name;
	if (const auto cache_it = module_cache.find(cache_key); cache_it != module_cache.end()) {
		return cache_it->second;
	}

	auto module = std::make_shared<ObjModule>(cache_key, factory_it->second());
	module_cache.insert_or_assign(cache_key, module);
	return module;
}

std::shared_ptr<ObjModule> Vm::load_module(const std::string &raw_path, const FilePos &location) {
	if (get_stdlib_registry().find(raw_path) != get_stdlib_registry().end()) {
		return load_stdlib_module(raw_path, location);
	}

	if (const auto precompiled_module_id = resolve_precompiled_import_id(raw_path); precompiled_module_id.has_value()) {
		const std::string cache_key = "pre:" + precompiled_module_id.value();
		if (const auto it = module_cache.find(cache_key); it != module_cache.end()) {
			return it->second;
		}

		if (loading_modules.find(cache_key) != loading_modules.end()) {
			throw CallError("Circular import detected for module '" + precompiled_module_id.value() + "'.", location);
		}

		const auto module_it = precompiled_modules.find(precompiled_module_id.value());
		if (module_it == precompiled_modules.end()) {
			throw NameError("Unknown precompiled module '" + precompiled_module_id.value() + "'.", location);
		}

		loading_modules.insert(cache_key);

		const std::string previous_source_path = source_path;
		const std::vector<std::string> previous_source_lines = source_lines;
		const auto previous_environment = environment;
		const auto previous_locals = locals;
		auto previous_exports = active_module_exports;

		try {
			for (const auto &[expr, depth] : module_it->second.resolved_locals) {
				locals.insert_or_assign(expr, depth);
			}

			source_path = module_it->second.source_path_hint.empty()
				? precompiled_module_id.value()
				: module_it->second.source_path_hint;
			source_lines.clear();
			if (!module_it->second.source_path_hint.empty()) {
				try {
					source_lines = read_module_lines(module_it->second.source_path_hint);
					source_lines_by_path.insert_or_assign(module_it->second.source_path_hint, source_lines);
				} catch (...) {
					source_lines.clear();
				}
			}

			auto module_env = std::make_shared<Environment>(globals);
			environment = module_env;

			std::unordered_map<std::string, Value> exports;
			active_module_exports = &exports;
			precompiled_module_stack.push_back(precompiled_module_id.value());

			for (const auto &statement : module_it->second.statements) {
				execute(statement);
			}

			precompiled_module_stack.pop_back();

			auto module = std::make_shared<ObjModule>(source_path, exports);
			module_cache.insert_or_assign(cache_key, module);

			environment = previous_environment;
			source_path = previous_source_path;
			source_lines = previous_source_lines;
			active_module_exports = previous_exports;
			loading_modules.erase(cache_key);

			return module;
		} catch (...) {
			if (!precompiled_module_stack.empty() && precompiled_module_stack.back() == precompiled_module_id.value()) {
				precompiled_module_stack.pop_back();
			}
			environment = previous_environment;
			locals = previous_locals;
			source_path = previous_source_path;
			source_lines = previous_source_lines;
			active_module_exports = previous_exports;
			loading_modules.erase(cache_key);
			throw;
		}
	}

	const std::string resolved_path = resolve_module_path(raw_path);
	if (const auto it = module_cache.find(resolved_path); it != module_cache.end()) {
		return it->second;
	}

	if (loading_modules.find(resolved_path) != loading_modules.end()) {
		throw CallError("Circular import detected for module '" + resolved_path + "'.", location);
	}

	loading_modules.insert(resolved_path);

	const std::string previous_source_path = source_path;
	const std::vector<std::string> previous_source_lines = source_lines;
	const auto previous_environment = environment;
	const auto previous_locals = locals;
	auto previous_exports = active_module_exports;

	try {
		FileLexer lexer(resolved_path);
		const std::vector<StmtPtr> statements = parse(lexer);
		if (lexer.had_error) {
			throw CallError("Lexing failed while loading module '" + resolved_path + "'.", location);
		}

		Resolver resolver;
		resolver.resolve(statements);
		if (resolver.had_error()) {
			std::ostringstream error;
			error << "Resolver errors in module '" << resolved_path << "': ";
			const auto &errors = resolver.get_errors();
			for (std::size_t i = 0; i < errors.size(); ++i) {
				error << errors[i];
				if (i + 1 < errors.size()) {
					error << " | ";
				}
			}
			throw CallError(error.str(), location);
		}

		for (const auto &[expr, depth] : resolver.get_locals()) {
			locals.insert_or_assign(expr, depth);
		}
		source_path = resolved_path;
		source_lines = read_module_lines(resolved_path);
		source_lines_by_path.insert_or_assign(resolved_path, source_lines);

		auto module_env = std::make_shared<Environment>(globals);
		environment = module_env;

		std::unordered_map<std::string, Value> exports;
		active_module_exports = &exports;

		for (const auto &statement : statements) {
			execute(statement);
		}

		auto module = std::make_shared<ObjModule>(resolved_path, exports);
		module_cache.insert_or_assign(resolved_path, module);

		environment = previous_environment;
		source_path = previous_source_path;
		source_lines = previous_source_lines;
		active_module_exports = previous_exports;
		loading_modules.erase(resolved_path);

		return module;
	} catch (...) {
		environment = previous_environment;
		locals = previous_locals;
		source_path = previous_source_path;
		source_lines = previous_source_lines;
		active_module_exports = previous_exports;
		loading_modules.erase(resolved_path);
		throw;
	}
}

std::string Vm::format_runtime_error(const RuntimeError &error) const {
	std::ostringstream oss;
	oss << kind_to_string(error.kind);
	std::string error_path = source_path;

	if (error.location.has_value()) {
		if (!error.location->path.empty()) {
			error_path = error.location->path;
		}

		if (!error_path.empty()) {
			oss << " [" << error_path << ':';
		} else {
			oss << " [";
		}

		oss << "line " << error.location->line_no << ", col " << error.location->column_no << "]";
	}

	oss << ": " << error.what();

	if (error.location.has_value()) {
		const std::size_t line_no = error.location->line_no;
		const std::size_t col_no = error.location->column_no;

		const std::vector<std::string> *lines = nullptr;
		if (!error_path.empty()) {
			if (const auto it = source_lines_by_path.find(error_path); it != source_lines_by_path.end()) {
				lines = &it->second;
			}
		}

		if (lines == nullptr) {
			lines = &source_lines;
		}

		if (line_no >= 1 && line_no <= lines->size()) {
			const std::string &line = (*lines)[line_no - 1];
			const std::size_t highlight_len = compute_highlight_length(line, col_no);
			oss << '\n' << line_no << " | " << line;

			oss << "\n  | ";
			if (col_no > 1) {
				oss << std::string(col_no - 1, ' ');
			}
			oss << '^';
			if (highlight_len > 1) {
				oss << std::string(highlight_len - 1, '~');
			}
		}
	}

	return oss.str();
}

bool Vm::invoke_main_if_present() {
	try {
		const Value main = globals->get("main");
		if (main.type != ValueType::Object) {
			throw CallError("'main' exists but is not callable.");
		}

		const auto callable = std::dynamic_pointer_cast<Callable>(main.as.object);
		if (callable == nullptr) {
			throw CallError("'main' exists but is not callable.");
		}

		if (callable->arity() != 0) {
			throw ArityError("'main' must take 0 arguments.");
		}

		callable->call(this, {});
		return true;
	} catch (const RuntimeError &error) {
		const std::string message = error.what();
		if (message.rfind("Undefined variable 'main'", 0) == 0) {
			return false;
		}

		std::cerr << format_runtime_error(error) << '\n';
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
			throw TypeError("Superclass must be a class.", stmt->name.file_pos);
		}

		super_class_obj = std::dynamic_pointer_cast<ObjClass>(super_class.as.object);
		if (super_class_obj == nullptr) {
			throw TypeError("Superclass must be a class.", stmt->name.file_pos);
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
		methods.insert_or_assign(method->name.literal.lexeme, std::make_shared<ObjFunction>(method, environment, is_init, stmt->name.literal.lexeme));
	}

	std::set<std::string> private_fields;
	std::vector<StmtPtr> field_init_body;
	field_init_body.reserve(stmt->private_fields.size());
	for (const auto &field : stmt->private_fields) {
		private_fields.insert(field.name.literal.lexeme);

		const Token self_token{TokenType::Identifier, "self", field.name.file_pos};
		const ExprPtr self_expr = std::make_shared<SelfExpr>(self_token);
		ExprPtr value_expr = field.init;
		if (value_expr == nullptr) {
			value_expr = std::make_shared<LiteralExpr>(Value::none());
		}

		field_init_body.push_back(
			std::make_shared<ExpressionStmt>(std::make_shared<SetExpr>(self_expr, field.name, value_expr))
		);
	}

	std::shared_ptr<ObjFunction> field_initializer = nullptr;
	if (!field_init_body.empty()) {
		const Token field_init_name{TokenType::Identifier, "<private-init>", stmt->name.file_pos};
		auto field_init_declaration = std::make_shared<FunctionStmt>(
			field_init_name,
			std::vector<FunctionStmt::Parameter>{},
			field_init_body,
			false
		);
		field_initializer = ObjFunction::basic(field_init_declaration, environment, stmt->name.literal.lexeme);
	}

	const auto klass = std::make_shared<ObjClass>(stmt->name.literal.lexeme, methods, private_fields, field_initializer);

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

    std::ostream &os = (value.type == ValueType::Object && value.as.object->get_type() == ObjType::Error) ? std::cerr : std::cout;
	value.print(os);
    os << std::endl;

	return Value::none();
}

Value Vm::visit_return_stmt(ReturnStmt *stmt) {
	Value value = Value::none();
	if (stmt->values.size() == 1) {
		value = evaluate(stmt->values[0]);
	} else if (stmt->values.size() > 1) {
		std::vector<Value> values;
		values.reserve(stmt->values.size());
		for (const ExprPtr &expr : stmt->values) {
			values.push_back(evaluate(expr));
		}
		value = Value::object(std::make_shared<List>(values));
	}

	throw Return(value);
}

Value Vm::visit_let_stmt(LetStmt *stmt) {
	if (stmt->names.empty()) {
		return Value::none();
	}

	if (stmt->names.size() == 1) {
		Value value = Value::none();
		if (!stmt->inits.empty()) {
			if (stmt->inits.size() != 1) {
				throw ArityError("Single-variable let expects at most one initializer.", stmt->keyword.file_pos);
			}
			value = evaluate(stmt->inits[0]);
		}

		environment->define(stmt->names[0].literal.lexeme, value);
		return Value::none();
	}

	std::vector<Value> resolved_values;
	resolved_values.reserve(stmt->names.size());

	if (stmt->inits.empty()) {
		for (std::size_t i = 0; i < stmt->names.size(); ++i) {
			resolved_values.push_back(Value::none());
		}
	} else if (stmt->inits.size() == 1) {
		const Value single = evaluate(stmt->inits[0]);
		if (single.type != ValueType::Object || single.as.object->get_type() != ObjType::List) {
			throw TypeError("Multi-variable let with one initializer expects a list value.", stmt->keyword.file_pos);
		}

		const auto list = std::static_pointer_cast<List>(single.as.object);
		if (list->elements.size() != stmt->names.size()) {
			throw ArityError("Destructuring count mismatch in let declaration.", stmt->keyword.file_pos);
		}

		resolved_values = list->elements;
	} else {
		if (stmt->inits.size() != stmt->names.size()) {
			throw ArityError("Initializer count mismatch in let declaration.", stmt->keyword.file_pos);
		}

		for (const ExprPtr &init : stmt->inits) {
			resolved_values.push_back(evaluate(init));
		}
	}

	for (std::size_t i = 0; i < stmt->names.size(); ++i) {
		environment->define(stmt->names[i].literal.lexeme, resolved_values[i]);
	}

	return Value::none();
}

Value Vm::visit_while_stmt(WhileStmt *stmt) {
	bool ran = false;
	while (is_truthy(evaluate(stmt->condition))) {
		ran = true;
		try {
			execute(stmt->body);
		} catch (const ContinueSignal &) {
			continue;
		} catch (const BreakSignal &) {
			break;
		}
	}

	if (ran && stmt->finally_clause != nullptr) {
		execute(stmt->finally_clause);
	}

	return Value::none();
}

Value Vm::visit_for_stmt(ForStmt *stmt) {
	const auto previous = environment;
	environment = std::make_shared<Environment>(environment);

	try {
		if (stmt->init != nullptr) {
			execute(stmt->init);
		}

		bool ran = false;
		while (stmt->condition == nullptr || is_truthy(evaluate(stmt->condition))) {
			ran = true;
			bool should_break = false;
			try {
				execute(stmt->body);
			} catch (const ContinueSignal &) {
				if (stmt->inc != nullptr) {
					(void) evaluate(stmt->inc);
				}
				continue;
			} catch (const BreakSignal &) {
				should_break = true;
			}

			if (should_break) {
				break;
			}

			if (stmt->inc != nullptr) {
				(void) evaluate(stmt->inc);
			}
		}

		if (ran && stmt->finally_clause != nullptr) {
			execute(stmt->finally_clause);
		}
	} catch (...) {
		environment = previous;
		throw;
	}

	environment = previous;
	return Value::none();
}

Value Vm::visit_do_while_stmt(DoWhileStmt *stmt) {
	bool ran = false;
	do {
		ran = true;
		try {
			execute(stmt->body);
		} catch (const ContinueSignal &) {
		} catch (const BreakSignal &) {
			break;
		}
	} while (is_truthy(evaluate(stmt->condition)));

	if (ran && stmt->finally_clause != nullptr) {
		execute(stmt->finally_clause);
	}

	return Value::none();
}

Value Vm::visit_break_stmt(BreakStmt *stmt) {
	(void) stmt;
	throw BreakSignal();
}

Value Vm::visit_continue_stmt(ContinueStmt *stmt) {
	(void) stmt;
	throw ContinueSignal();
}

Value Vm::visit_def_stmt(DefStmt *stmt) {
	auto declaration = std::make_shared<FunctionStmt>(stmt->new_keyword, stmt->params, stmt->body, false);
	if (stmt->blocks.has_value()) {
		declaration = std::make_shared<FunctionStmt>(stmt->new_keyword, stmt->params, stmt->blocks.value(), stmt->body, false);
	}

	environment->define(stmt->new_keyword.literal.lexeme, Value::object(ObjFunction::basic(declaration, environment)));
	return Value::none();
}

Value Vm::visit_import_stmt(ImportStmt *stmt) {
	const auto module = load_module(stmt->path.literal.lexeme, stmt->path.file_pos);
	environment->define(stmt->alias.literal.lexeme, Value::object(module));
	return Value::none();
}

Value Vm::visit_from_import_stmt(FromImportStmt *stmt) {
	const auto module = load_module(stmt->path.literal.lexeme, stmt->path.file_pos);
	for (const auto &import_name : stmt->names) {
		if (!module->has(import_name.name.literal.lexeme)) {
			throw NameError("Module '" + module->path + "' does not export '" + import_name.name.literal.lexeme + "'.", import_name.name.file_pos);
		}
		environment->define(import_name.alias.literal.lexeme, module->get(import_name.name.literal.lexeme));
	}
	return Value::none();
}

Value Vm::visit_export_stmt(ExportStmt *stmt) {
	execute(stmt->declaration);

	if (active_module_exports == nullptr) {
		return Value::none();
	}

	if (const auto function_stmt = std::dynamic_pointer_cast<FunctionStmt>(stmt->declaration); function_stmt != nullptr) {
		active_module_exports->insert_or_assign(function_stmt->name.literal.lexeme, environment->get(function_stmt->name.literal.lexeme));
		return Value::none();
	}

	if (const auto def_stmt = std::dynamic_pointer_cast<DefStmt>(stmt->declaration); def_stmt != nullptr) {
		active_module_exports->insert_or_assign(def_stmt->new_keyword.literal.lexeme, environment->get(def_stmt->new_keyword.literal.lexeme));
		return Value::none();
	}

	if (const auto class_stmt = std::dynamic_pointer_cast<ClassStmt>(stmt->declaration); class_stmt != nullptr) {
		active_module_exports->insert_or_assign(class_stmt->name.literal.lexeme, environment->get(class_stmt->name.literal.lexeme));
		return Value::none();
	}

	if (const auto let_stmt = std::dynamic_pointer_cast<LetStmt>(stmt->declaration); let_stmt != nullptr) {
		for (const auto &name : let_stmt->names) {
			active_module_exports->insert_or_assign(name.literal.lexeme, environment->get(name.literal.lexeme));
		}
	}

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

			throw TypeError("Operands to '+' must both be numbers or strings.", expr->op.file_pos);

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

		case TokenType::CouldBe:
			return Value::boolean(is_type_match(left, right));

		default:
			break;
	}

	return Value::none();
}

Value Vm::visit_call_expr(CallExpr *expr) {
	const Value callee = evaluate(expr->callee);

	if (callee.type != ValueType::Object) {
		throw CallError("Can only call callable objects.", expr->paren.file_pos);
	}

	const auto callable = std::dynamic_pointer_cast<Callable>(callee.as.object);
	if (callable == nullptr) {
		throw CallError("Can only call callable objects.", expr->paren.file_pos);
	}

	std::vector<Value> args;
	args.reserve(expr->arguments.size());

	if (const auto function = std::dynamic_pointer_cast<ObjFunction>(callee.as.object); function != nullptr) {
		std::vector<ExprPtr> positional_arguments;
		std::map<std::string, ExprPtr> named_block_arguments;

		for (const auto &argument : expr->arguments) {
			if (const auto named_block = std::dynamic_pointer_cast<NamedBlockExpr>(argument); named_block != nullptr) {
				named_block_arguments.insert_or_assign(named_block->name.literal.lexeme, named_block->value);
			} else {
				positional_arguments.push_back(argument);
			}
		}

		const std::size_t param_count = function->declaration->params.size();
		std::size_t regular_param_count = param_count;
		bool has_variadic_param = false;
		for (std::size_t i = 0; i < param_count; ++i) {
			if (function->declaration->params[i].is_variadic) {
				has_variadic_param = true;
				regular_param_count = i;
				break;
			}
		}
		const std::size_t block_count = function->declaration->blocks.has_value() ? function->declaration->blocks->size() : 0;
		const std::size_t required_total = regular_param_count;
		const std::size_t max_total = has_variadic_param ? static_cast<std::size_t>(-1) : (regular_param_count + block_count);

		if (positional_arguments.size() < required_total || positional_arguments.size() > max_total) {
			throw ArityError("Argument count mismatch.", expr->paren.file_pos);
		}

		const std::size_t extra_after_regular = positional_arguments.size() - regular_param_count;
		const std::size_t positional_block_count = has_variadic_param
			? std::min(block_count, extra_after_regular)
			: (positional_arguments.size() - regular_param_count);
		const std::size_t variadic_count = has_variadic_param ? (extra_after_regular - positional_block_count) : 0;

		for (std::size_t i = 0; i < regular_param_count; ++i) {
			const ExprPtr &argument = positional_arguments[i];
			if (function->declaration->params[i].name.type == TokenType::ExprIdentifier) {
				if (std::dynamic_pointer_cast<FunctionExpr>(argument) != nullptr) {
					args.push_back(evaluate(argument));
				} else {
					const Token literal_name{TokenType::Identifier, "<expr-literal>", FilePos{}};
					std::vector<StmtPtr> body;
					body.push_back(std::make_shared<ExpressionStmt>(argument));
					const auto declaration = std::make_shared<FunctionStmt>(literal_name, std::vector<FunctionStmt::Parameter>{}, body, false);
					args.push_back(Value::object(ObjFunction::basic(declaration, environment)));
				}
			} else {
				args.push_back(evaluate(argument));
			}
		}

		if (has_variadic_param) {
			for (std::size_t i = 0; i < variadic_count; ++i) {
				args.push_back(evaluate(positional_arguments[regular_param_count + i]));
			}
		}

		if (block_count > 0) {
			std::vector<Value> resolved_blocks(block_count, Value::none());
			std::size_t positional_block_idx = regular_param_count + variadic_count;

			for (std::size_t i = 0; i < positional_block_count && positional_block_idx < positional_arguments.size(); ++i, ++positional_block_idx) {
				resolved_blocks[i] = evaluate(positional_arguments[positional_block_idx]);
			}

			std::set<std::string> used_named_blocks;
			for (std::size_t i = 0; i < block_count; ++i) {
				const auto &block = function->declaration->blocks.value()[i];
				std::string expected_name;

				if (block.expect.has_value()) {
					expected_name = block.expect->literal.lexeme;
				} else if (!block.name.literal.lexeme.empty() && block.name.literal.lexeme[0] == '$') {
					expected_name = block.name.literal.lexeme.substr(1);
				} else {
					expected_name = block.name.literal.lexeme;
				}

				if (const auto it = named_block_arguments.find(expected_name); it != named_block_arguments.end()) {
					resolved_blocks[i] = evaluate(it->second);
					used_named_blocks.insert(expected_name);
				}
			}

			for (const auto &[name, value] : named_block_arguments) {
				(void) value;
				if (used_named_blocks.find(name) == used_named_blocks.end()) {
					throw CallError("Unknown named block '" + name + "'.", expr->paren.file_pos);
				}
			}

			for (const auto &block : resolved_blocks) {
				args.push_back(block);
			}
		}
	} else {
		for (const auto &argument : expr->arguments) {
			args.push_back(evaluate(argument));
		}

		if (const auto klass = std::dynamic_pointer_cast<ObjClass>(callee.as.object); klass != nullptr) {
			const auto init = klass->find_method("init");
			const int required = callable->arity();

			bool has_variadic_param = false;
			if (init != nullptr) {
				for (const auto &param : init->declaration->params) {
					if (param.is_variadic) {
						has_variadic_param = true;
						break;
					}
				}
			}

			const int actual = static_cast<int>(args.size());
			if ((has_variadic_param && actual < required) || (!has_variadic_param && actual != required)) {
				throw ArityError("Argument count mismatch.", expr->paren.file_pos);
			}
		} else if (static_cast<int>(args.size()) != callable->arity()) {
			throw ArityError("Argument count mismatch.", expr->paren.file_pos);
		}
	}

	return callable->call(this, args);
}

Value Vm::visit_function_expr(FunctionExpr *expr) {
	const Token literal_name{TokenType::Identifier, "<literal>", FilePos{}};
	auto declaration = std::make_shared<FunctionStmt>(literal_name, std::vector<FunctionStmt::Parameter>{}, expr->body, false);
	return Value::object(ObjFunction::basic(declaration, environment));
}

Value Vm::visit_index_expr(IndexExpr *expr) {
	const Value obj = evaluate(expr->obj);
	const Value key = evaluate(expr->key);

	if (obj.type != ValueType::Object) {
		throw TypeError("Only objects are indexable.", expr->bracket.file_pos);
	}

	return obj.as.object->index(key);
}

Value Vm::visit_list_expr(ListExpr *expr) {
	std::vector<Value> elements;
	elements.reserve(expr->elements.size());

	for (const ExprPtr &element : expr->elements) {
		elements.push_back(evaluate(element));
	}

	return Value::object(std::make_shared<List>(elements));
}

Value Vm::visit_named_block_expr(NamedBlockExpr *expr) {
	return evaluate(expr->value);
}

Value Vm::visit_get_expr(GetExpr *expr) {
	const Value obj = evaluate(expr->obj);
	if (obj.type != ValueType::Object) {
		throw TypeError("Only instances have properties.", expr->name.file_pos);
	}

	if (const auto module = std::dynamic_pointer_cast<ObjModule>(obj.as.object); module != nullptr) {
		if (!module->has(expr->name.literal.lexeme)) {
			throw PropertyError("Undefined exported name '" + expr->name.literal.lexeme + "'.", expr->name.file_pos);
		}
		return module->get(expr->name.literal.lexeme);
	}

	const auto instance = std::dynamic_pointer_cast<ObjInstance>(obj.as.object);
	if (instance == nullptr) {
		throw TypeError("Only instances have properties.", expr->name.file_pos);
	}

	return instance->get(expr->name.literal.lexeme, current_class_access());
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
	} else if (expr->op.type == TokenType::Or) {
		if (is_truthy(left)) {
			return left;
		}
	} else {
		throw RuntimeError(RuntimeErrorKind::Runtime, "Invalid logical operator token.", expr->op.file_pos);
	}

	return evaluate(expr->right);
}

Value Vm::visit_set_expr(SetExpr *expr) {
	const Value obj = evaluate(expr->obj);
	if (obj.type != ValueType::Object) {
		throw TypeError("Only instances have fields.", expr->name.file_pos);
	}

	const auto instance = std::dynamic_pointer_cast<ObjInstance>(obj.as.object);
	if (instance == nullptr) {
		throw TypeError("Only instances have fields.", expr->name.file_pos);
	}

	const Value value = evaluate(expr->value);
	instance->set(expr->name.literal.lexeme, value, current_class_access());
	return value;
}

Value Vm::visit_super_expr(SuperExpr *expr) {
	const auto super_it = locals.find(expr);
	if (super_it == locals.end()) {
		throw NameError("Unable to resolve 'super'.", expr->keyword.file_pos);
	}

	const int distance = super_it->second;
	Value super = environment->get_at(distance, "super");
	Value self = environment->get_at(distance - 1, "self");

	const auto super_class = std::dynamic_pointer_cast<ObjClass>(super.as.object);
	const auto instance = std::dynamic_pointer_cast<ObjInstance>(self.as.object);

	const auto method = super_class->find_method(expr->method.literal.lexeme);
	if (method == nullptr) {
		throw PropertyError("Undefined property '" + expr->method.literal.lexeme + "'.", expr->method.file_pos);
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

Value Vm::visit_update_expr(UpdateExpr *expr) {
	const bool is_increment = expr->op.type == TokenType::PlusPlus;
	const double delta = is_increment ? 1.0 : -1.0;

	if (const auto variable = std::dynamic_pointer_cast<VariableExpr>(expr->target); variable != nullptr) {
		const Value current = lookup_variable(variable->name, expr);
		assert_number_operand(expr->op, current);

		const Value updated = Value::number(current.as.number + delta);
		if (const auto it = locals.find(expr); it != locals.end()) {
			environment->assign_at(it->second, variable->name.literal.lexeme, updated);
		} else {
			globals->assign(variable->name.literal.lexeme, updated);
		}

		return expr->is_prefix ? updated : current;
	}

	if (const auto get = std::dynamic_pointer_cast<GetExpr>(expr->target); get != nullptr) {
		const Value obj = evaluate(get->obj);
		if (obj.type != ValueType::Object) {
			throw TypeError("Only instances have fields.", get->name.file_pos);
		}

		const auto instance = std::dynamic_pointer_cast<ObjInstance>(obj.as.object);
		if (instance == nullptr) {
			throw TypeError("Only instances have fields.", get->name.file_pos);
		}

		const Value current = instance->get(get->name.literal.lexeme, current_class_access());
		assert_number_operand(expr->op, current);

		const Value updated = Value::number(current.as.number + delta);
		instance->set(get->name.literal.lexeme, updated, current_class_access());
		return expr->is_prefix ? updated : current;
	}

	throw TypeError("Invalid target for update operator.", expr->op.file_pos);
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

	return environment->get(name.literal.lexeme);
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

bool Vm::is_type_match(const Value &left, const Value &right) {
	if (right.type != ValueType::Object) {
		return false;
	}

	const auto right_class = std::dynamic_pointer_cast<ObjClass>(right.as.object);
	if (right_class == nullptr) {
		return false;
	}

	if (left.type != ValueType::Object) {
		return false;
	}

	if (const auto left_instance = std::dynamic_pointer_cast<ObjInstance>(left.as.object); left_instance != nullptr) {
		return left_instance->klass->name == right_class->name;
	}

	if (const auto left_class = std::dynamic_pointer_cast<ObjClass>(left.as.object); left_class != nullptr) {
		return left_class->name == right_class->name;
	}

	return false;
}

void Vm::assert_number_operand(const Token &op, const Value &value) {
	(void) op;
	if (value.type != ValueType::Number) {
		throw TypeError("Operand must be a number.", op.file_pos);
	}
}

void Vm::assert_number_operands(const Token &op, const Value &left, const Value &right) {
	(void) op;
	if (left.type != ValueType::Number || right.type != ValueType::Number) {
		throw TypeError("Operands must be numbers.", op.file_pos);
	}
}

std::string Vm::stringify(const Value &value) {
	std::ostringstream oss;
	value.print(oss);
	return oss.str();
}

void Vm::push_class_access(const std::string &class_name) {
	class_access_stack.push_back(class_name);
}

void Vm::pop_class_access() {
	if (!class_access_stack.empty()) {
		class_access_stack.pop_back();
	}
}

std::string Vm::current_class_access() const {
	if (class_access_stack.empty()) {
		return "";
	}
	return class_access_stack.back();
}
