#include "cli.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "artifact_loader.h"
#include "json_builder.h"
#include "../lexing/lexer.h"
#include "../parsing/parser.h"
#include "../resolving/resolver.h"
#include "../runtime/stdlibs/io.h"
#include "../runtime/stdlibs/registry.h"
#include "../runtime/vm.h"

#define Version "0.1.0"

namespace {

void print_usage(const std::string &program_name) {
    std::cerr << "Usage:\n";
    std::cerr << "  " << program_name << " --repl\n";
    std::cerr << "  " << program_name << " --build <entry_script> [artifact_path]\n";
    std::cerr << "  " << program_name << " --run <script_or_artifact> [args...]\n";
    std::cerr << "  " << program_name << " --help\n";
    std::cerr << "  " << program_name << " --version\n";
    std::cerr << "\n";
    std::cerr << "Default (backward-compatible):\n";
    std::cerr << "  " << program_name << " <script_path>\n";
}

struct BuildModule {
    std::string path;
    std::vector<std::string> imports;
    std::vector<std::pair<std::string, std::string>> import_map;
    std::vector<std::string> external_imports;
    std::vector<StmtPtr> ast;
    std::unordered_map<const Expr *, int> resolved_locals;
};

std::string json_escape(const std::string &value) {
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

std::string unescape_json_string(const std::string &value) {
    std::ostringstream oss;
    bool escaping = false;
    for (const char ch : value) {
        if (!escaping) {
            if (ch == '\\') {
                escaping = true;
            } else {
                oss << ch;
            }
            continue;
        }

        switch (ch) {
            case 'n': oss << '\n'; break;
            case 'r': oss << '\r'; break;
            case 't': oss << '\t'; break;
            case '\\': oss << '\\'; break;
            case '"': oss << '"'; break;
            default: oss << ch; break;
        }
        escaping = false;
    }
    return oss.str();
}

std::string resolve_file_import(const std::string &current_module_path, const std::string &raw_path) {
    namespace fs = std::filesystem;

    fs::path import_path(raw_path);
    fs::path candidate;
    if (import_path.is_absolute()) {
        candidate = import_path;
    } else {
        candidate = fs::path(current_module_path).parent_path() / import_path;
    }

    if (!fs::exists(candidate)) {
        return "";
    }

    return fs::weakly_canonical(candidate).string();
}

bool has_artifact_extension(const std::string &path) {
    if (path.size() < std::char_traits<char>::length(ArtifactExtension)) {
        return false;
    }

    return path.compare(path.size() - std::char_traits<char>::length(ArtifactExtension),
        std::char_traits<char>::length(ArtifactExtension), ArtifactExtension) == 0;
}

void collect_raw_imports_from_stmt(const StmtPtr &stmt, std::vector<std::string> &out) {
    if (stmt == nullptr) {
        return;
    }

    if (const auto import_stmt = std::dynamic_pointer_cast<ImportStmt>(stmt); import_stmt != nullptr) {
        out.push_back(import_stmt->path.literal.lexeme);
        return;
    }

    if (const auto from_import_stmt = std::dynamic_pointer_cast<FromImportStmt>(stmt); from_import_stmt != nullptr) {
        out.push_back(from_import_stmt->path.literal.lexeme);
        return;
    }

    if (const auto block_stmt = std::dynamic_pointer_cast<BlockStmt>(stmt); block_stmt != nullptr) {
        for (const auto &nested : block_stmt->statements) {
            collect_raw_imports_from_stmt(nested, out);
        }
        return;
    }

    if (const auto function_stmt = std::dynamic_pointer_cast<FunctionStmt>(stmt); function_stmt != nullptr) {
        for (const auto &nested : function_stmt->body) {
            collect_raw_imports_from_stmt(nested, out);
        }
        return;
    }

    if (const auto def_stmt = std::dynamic_pointer_cast<DefStmt>(stmt); def_stmt != nullptr) {
        for (const auto &nested : def_stmt->body) {
            collect_raw_imports_from_stmt(nested, out);
        }
        return;
    }

    if (const auto class_stmt = std::dynamic_pointer_cast<ClassStmt>(stmt); class_stmt != nullptr) {
        for (const auto &method : class_stmt->methods) {
            for (const auto &nested : method->body) {
                collect_raw_imports_from_stmt(nested, out);
            }
        }
        return;
    }

    if (const auto export_stmt = std::dynamic_pointer_cast<ExportStmt>(stmt); export_stmt != nullptr) {
        collect_raw_imports_from_stmt(export_stmt->declaration, out);
        return;
    }

    if (const auto if_stmt = std::dynamic_pointer_cast<IfStmt>(stmt); if_stmt != nullptr) {
        collect_raw_imports_from_stmt(if_stmt->thenBranch, out);
        collect_raw_imports_from_stmt(if_stmt->elseBranch, out);
        return;
    }

    if (const auto while_stmt = std::dynamic_pointer_cast<WhileStmt>(stmt); while_stmt != nullptr) {
        collect_raw_imports_from_stmt(while_stmt->body, out);
        collect_raw_imports_from_stmt(while_stmt->finally_clause, out);
        return;
    }

    if (const auto for_stmt = std::dynamic_pointer_cast<ForStmt>(stmt); for_stmt != nullptr) {
        collect_raw_imports_from_stmt(for_stmt->init, out);
        collect_raw_imports_from_stmt(for_stmt->body, out);
        collect_raw_imports_from_stmt(for_stmt->finally_clause, out);
        return;
    }

    if (const auto do_while_stmt = std::dynamic_pointer_cast<DoWhileStmt>(stmt); do_while_stmt != nullptr) {
        collect_raw_imports_from_stmt(do_while_stmt->body, out);
        collect_raw_imports_from_stmt(do_while_stmt->finally_clause, out);
    }
}

bool analyze_module_recursive(
    const std::string &module_path,
    std::map<std::string, BuildModule> &modules,
    std::set<std::string> &visiting,
    std::vector<std::string> &errors
) {
    if (modules.find(module_path) != modules.end()) {
        return true;
    }

    if (visiting.find(module_path) != visiting.end()) {
        errors.push_back("Circular import detected while building: " + module_path);
        return false;
    }

    visiting.insert(module_path);

    FileLexer lexer(module_path);
    const std::vector<StmtPtr> statements = parse(lexer);
    if (lexer.had_error) {
        errors.push_back("Lexing failed for " + module_path);
        visiting.erase(module_path);
        return false;
    }

    Resolver resolver;
    resolver.resolve(statements);
    if (resolver.had_error()) {
        for (const auto &error : resolver.get_errors()) {
            errors.push_back(error);
        }
        visiting.erase(module_path);
        return false;
    }

    BuildModule module;
    module.path = module_path;
    module.ast = statements;
    module.resolved_locals = resolver.get_locals();

    std::vector<std::string> raw_imports;
    for (const auto &statement : statements) {
        collect_raw_imports_from_stmt(statement, raw_imports);
    }

    const auto &stdlib_registry = get_stdlib_registry();
    for (const std::string &raw_import : raw_imports) {
        if (stdlib_registry.find(raw_import) != stdlib_registry.end()) {
            module.external_imports.push_back(raw_import);
            continue;
        }

        const std::string resolved = resolve_file_import(module_path, raw_import);
        if (resolved.empty()) {
            module.external_imports.push_back(raw_import);
            continue;
        }

        if (has_artifact_extension(resolved)) {
            module.external_imports.push_back(raw_import);
            continue;
        }

        module.imports.push_back(resolved);
        module.import_map.emplace_back(raw_import, resolved);
        if (!analyze_module_recursive(resolved, modules, visiting, errors)) {
            visiting.erase(module_path);
            return false;
        }
    }

    modules.insert_or_assign(module_path, module);
    visiting.erase(module_path);
    return true;
}

bool write_build_artifact(
    const std::string &artifact_path,
    const std::string &entry_path,
    const std::map<std::string, BuildModule> &modules,
    std::string &error
) {
    std::ofstream out(artifact_path);
    if (!out.is_open()) {
        error = "Could not write artifact to '" + artifact_path + "'.";
        return false;
    }

    out << "{\n";
    out << "  \"schema\": \"dscript.build.v1\",\n";
    out << "  \"version\": \"" << Version << "\",\n";
    std::map<std::string, std::string> module_ids;
    std::size_t module_idx = 0;
    for (const auto &[path, module] : modules) {
        (void) module;
        module_ids.insert_or_assign(path, "m" + std::to_string(module_idx++));
    }

    out << "  \"entryModule\": \"" << module_ids[entry_path] << "\",\n";
    out << "  \"modules\": [\n";

    std::size_t idx = 0;
    for (const auto &[path, module] : modules) {
        (void) path;
        out << "    {\n";
        out << "      \"id\": \"" << module_ids[module.path] << "\",\n";
        out << "      \"path\": \"" << json_escape(module.path) << "\",\n";

        out << "      \"imports\": [";
        for (std::size_t i = 0; i < module.imports.size(); ++i) {
            if (i > 0) out << ", ";
            out << "\"" << json_escape(module_ids[module.imports[i]]) << "\"";
        }
        out << "],\n";

        out << "      \"importMap\": [";
        for (std::size_t i = 0; i < module.import_map.size(); ++i) {
            const auto &[raw, resolved_path] = module.import_map[i];
            if (i > 0) out << ", ";
            out << "{\"raw\":\"" << json_escape(raw) << "\",\"module\":\"" << json_escape(module_ids[resolved_path]) << "\"}";
        }
        out << "],\n";

        out << "      \"externalImports\": [";
        for (std::size_t i = 0; i < module.external_imports.size(); ++i) {
            if (i > 0) out << ", ";
            out << "\"" << json_escape(module.external_imports[i]) << "\"";
        }
        out << "],\n";

        JsonAstBuilder builder(module.resolved_locals);
        out << "      \"ast\": " << builder.serialize_statements(module.ast) << ",\n";
        out << "      \"resolvedLocals\": " << builder.serialize_resolved_locals() << "\n";

        out << "    }";
        if (++idx < modules.size()) {
            out << ",";
        }
        out << "\n";
    }

    out << "  ]\n";
    out << "}\n";
    return true;
}

bool parse_artifact_entry(const std::string &artifact_path, std::string &entry_out, std::string &error) {
    std::ifstream in(artifact_path);
    if (!in.is_open()) {
        error = "Could not open artifact '" + artifact_path + "'.";
        return false;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    const std::string json = buffer.str();

    const std::size_t key_pos = json.find("\"entryModule\"");
    if (key_pos == std::string::npos) {
        error = "Artifact missing required 'entryModule' field.";
        return false;
    }

    const std::size_t colon_pos = json.find(':', key_pos);
    if (colon_pos == std::string::npos) {
        error = "Artifact contains malformed 'entryModule' field.";
        return false;
    }

    std::size_t quote_start = json.find('"', colon_pos + 1);
    if (quote_start == std::string::npos) {
        error = "Artifact contains malformed 'entryModule' value.";
        return false;
    }
    ++quote_start;

    std::ostringstream raw;
    bool escaping = false;
    for (std::size_t i = quote_start; i < json.size(); ++i) {
        const char ch = json[i];
        if (!escaping) {
            if (ch == '\\') {
                escaping = true;
                raw << ch;
                continue;
            }
            if (ch == '"') {
                entry_out = unescape_json_string(raw.str());
                return true;
            }
            raw << ch;
            continue;
        }

        raw << ch;
        escaping = false;
    }

    error = "Artifact contains unterminated 'entryModule' string.";
    return false;
}

bool interpret_script(const std::string &script_path, const std::vector<std::string> &program_args) {
    FileLexer lexer(script_path);
    const std::vector<StmtPtr> statements = parse(lexer);
    if (lexer.had_error) {
        std::cerr << "Lexing failed for " << script_path << "\n";
        return false;
    }

    Resolver resolver;
    resolver.resolve(statements);
    if (resolver.had_error()) {
        for (const auto &error : resolver.get_errors()) {
            std::cerr << error << '\n';
        }
        return false;
    }

    set_io_program_args(program_args);
    Vm vm;
    vm.set_source(script_path);
    vm.set_locals(resolver.get_locals());
    vm.interpret(statements);
    (void) vm.invoke_main_if_present();
    return true;
}

void repl() {
    std::cout << "dscript REPL (type 'exit' to quit)\n";

    Vm vm;
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line == "exit" || line == "quit") {
            break;
        }

        if (line.empty()) {
            continue;
        }

        StringLexer lexer(line);
        const std::vector<StmtPtr> statements = parse(lexer);
        if (lexer.had_error) {
            std::cerr << "Lexing failed for REPL input\n";
            continue;
        }

        Resolver resolver;
        resolver.resolve(statements);
        if (resolver.had_error()) {
            for (const auto &error : resolver.get_errors()) {
                std::cerr << error << '\n';
            }
            continue;
        }

        vm.set_source("<repl>");
        vm.set_locals(resolver.get_locals());
        vm.interpret(statements);
    }
}

bool build(const std::string &script_path, const std::string &artifact_path) {
    namespace fs = std::filesystem;

    if (!fs::exists(script_path)) {
        std::cerr << "Entry script not found: " << script_path << "\n";
        return false;
    }

    const std::string entry = fs::weakly_canonical(fs::path(script_path)).string();

    std::map<std::string, BuildModule> modules;
    std::set<std::string> visiting;
    std::vector<std::string> errors;

    const bool ok = analyze_module_recursive(entry, modules, visiting, errors);
    if (!ok) {
        for (const auto &error : errors) {
            std::cerr << error << '\n';
        }
        return false;
    }

    std::string write_error;
    if (!write_build_artifact(artifact_path, entry, modules, write_error)) {
        std::cerr << write_error << '\n';
        return false;
    }

    std::cout << "Entry: " << entry << "\n";
    std::cout << "Built artifact: " << artifact_path << "\n";
    std::cout << "... with " << modules.size() << " modules\n";
    return true;
}

bool run(const std::string &script_or_artifact_path, const std::vector<std::string> &program_args) {
    namespace fs = std::filesystem;

    std::string script_path = script_or_artifact_path;
    const std::string artifact_extension = ArtifactExtension;
    if (script_or_artifact_path.size() >= artifact_extension.size() &&
        script_or_artifact_path.substr(script_or_artifact_path.size() - artifact_extension.size()) == artifact_extension) {
        ArtifactProgram program;
        std::string load_error;
        if (!load_artifact_program(script_or_artifact_path, program, load_error)) {
            std::cerr << load_error << '\n';
            return false;
        }

        const auto entry_it = program.modules.find(program.entry_module);
        if (entry_it == program.modules.end()) {
            std::cerr << "Artifact entry module missing: " << program.entry_module << '\n';
            return false;
        }

        set_io_program_args(program_args);
        Vm vm;
        vm.clear_precompiled_modules();
        for (const auto &[module_id, module] : program.modules) {
            vm.register_precompiled_module(
                module_id,
                module.ast,
                module.resolved_locals,
                module.import_map,
                module.source_path
            );
        }

        const ArtifactModule &entry_module = entry_it->second;
        vm.set_source(entry_module.source_path.empty() ? script_or_artifact_path : entry_module.source_path);
        vm.set_locals(entry_module.resolved_locals);
        vm.interpret(entry_module.ast);
        (void) vm.invoke_main_if_present();
        return true;
    }

    if (!fs::exists(script_path)) {
        std::cerr << "Script not found: " << script_path << "\n";
        return false;
    }

    return interpret_script(script_path, program_args);
}

bool switch_on_mode(
    const std::string &program_name,
    const std::string &mode,
    const std::string &script_path,
    const std::string &artifact_path,
    const std::vector<std::string> &program_args
) {
    if (mode == "--repl") {
        repl();
        return true;
    }

    if (mode == "--build") {
        if (script_path.empty()) {
            print_usage(program_name);
            return false;
        }
        std::string out_path = artifact_path;
        if (out_path.empty()) {
            std::filesystem::path default_path(script_path);
            default_path.replace_extension(ArtifactExtension);
            out_path = default_path.string();
        }
        return build(script_path, out_path);
    }

    if (mode == "--run") {
        if (script_path.empty()) {
            print_usage(program_name);
            return false;
        }
        return run(script_path, program_args);
    }

    if (mode == "--help") {
        print_usage(program_name);
        return true;
    }

    if (mode == "--version") {
        std::cout << Version << '\n';
        return true;
    }

    print_usage(program_name);
    return false;
}

} // namespace

int run_cli(const int argc, char **argv) {
    const std::string program_name = argc > 0 ? argv[0] : "dscript";

    if (argc <= 1) {
        print_usage(program_name);
        return 1;
    }

    const std::string first_arg = argv[1];
    if (!first_arg.empty() && first_arg[0] == '-') {
        const std::string script_path = argc > 2 ? argv[2] : "";
        const std::string artifact_path = argc > 3 ? argv[3] : "";
        std::vector<std::string> program_args;
        if (first_arg == "--run") {
            for (int i = 3; i < argc; ++i) {
                program_args.emplace_back(argv[i]);
            }
        }
        return switch_on_mode(program_name, first_arg, script_path, artifact_path, program_args) ? 0 : 1;
    }

    std::vector<std::string> program_args;
    for (int i = 2; i < argc; ++i) {
        program_args.emplace_back(argv[i]);
    }
    return run(first_arg, program_args) ? 0 : 1;
}
