#include "artifact_loader.h"

#include <cctype>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../runtime/objs.h"

namespace {

class JsonValue {
public:
    enum class Type {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object,
    };

    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue>;

    JsonValue() : type_(Type::Null), bool_(false), number_(0.0) {}
    explicit JsonValue(const bool value) : type_(Type::Bool), bool_(value), number_(0.0) {}
    explicit JsonValue(const double value) : type_(Type::Number), bool_(false), number_(value) {}
    explicit JsonValue(std::string value) : type_(Type::String), bool_(false), number_(0.0), string_(std::move(value)) {}
    explicit JsonValue(Array value) : type_(Type::Array), bool_(false), number_(0.0), array_(std::move(value)) {}
    explicit JsonValue(Object value) : type_(Type::Object), bool_(false), number_(0.0), object_(std::move(value)) {}

    Type type() const { return type_; }
    bool as_bool() const { return bool_; }
    double as_number() const { return number_; }
    const std::string &as_string() const { return string_; }
    const Array &as_array() const { return array_; }
    const Object &as_object() const { return object_; }

private:
    Type type_;
    bool bool_;
    double number_;
    std::string string_;
    Array array_;
    Object object_;
};

class JsonParser {
public:
    explicit JsonParser(std::string source) : source_(std::move(source)), index_(0) {}

    JsonValue parse() {
        skip_ws();
        JsonValue value = parse_value();
        skip_ws();
        if (index_ != source_.size()) {
            throw std::runtime_error("Unexpected trailing characters in JSON.");
        }
        return value;
    }

private:
    std::string source_;
    std::size_t index_;

    static char hex_to_char(const char c) {
        if (c >= '0' && c <= '9') return static_cast<char>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<char>(10 + (c - 'a'));
        if (c >= 'A' && c <= 'F') return static_cast<char>(10 + (c - 'A'));
        throw std::runtime_error("Invalid hex escape in JSON string.");
    }

    void skip_ws() {
        while (index_ < source_.size() && std::isspace(static_cast<unsigned char>(source_[index_])) != 0) {
            ++index_;
        }
    }

    bool match(const char expected) {
        if (index_ >= source_.size() || source_[index_] != expected) {
            return false;
        }
        ++index_;
        return true;
    }

    void expect(const char expected) {
        if (!match(expected)) {
            throw std::runtime_error(std::string("Expected '") + expected + "' while parsing JSON.");
        }
    }

    void expect_keyword(const std::string &keyword) {
        for (const char c : keyword) {
            if (index_ >= source_.size() || source_[index_] != c) {
                throw std::runtime_error("Invalid JSON literal.");
            }
            ++index_;
        }
    }

    JsonValue parse_value() {
        if (index_ >= source_.size()) {
            throw std::runtime_error("Unexpected end of JSON input.");
        }

        const char c = source_[index_];
        if (c == '{') {
            return parse_object();
        }
        if (c == '[') {
            return parse_array();
        }
        if (c == '"') {
            return JsonValue(parse_string());
        }
        if (c == 't') {
            expect_keyword("true");
            return JsonValue(true);
        }
        if (c == 'f') {
            expect_keyword("false");
            return JsonValue(false);
        }
        if (c == 'n') {
            expect_keyword("null");
            return JsonValue();
        }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c)) != 0) {
            return JsonValue(parse_number());
        }

        throw std::runtime_error("Invalid JSON value.");
    }

    JsonValue parse_object() {
        expect('{');
        JsonValue::Object object;
        skip_ws();
        if (match('}')) {
            return JsonValue(std::move(object));
        }

        while (true) {
            skip_ws();
            if (index_ >= source_.size() || source_[index_] != '"') {
                throw std::runtime_error("Expected object key string in JSON.");
            }
            const std::string key = parse_string();
            skip_ws();
            expect(':');
            skip_ws();
            object.insert_or_assign(key, parse_value());
            skip_ws();
            if (match('}')) {
                break;
            }
            expect(',');
            skip_ws();
        }

        return JsonValue(std::move(object));
    }

    JsonValue parse_array() {
        expect('[');
        JsonValue::Array array;
        skip_ws();
        if (match(']')) {
            return JsonValue(std::move(array));
        }

        while (true) {
            skip_ws();
            array.push_back(parse_value());
            skip_ws();
            if (match(']')) {
                break;
            }
            expect(',');
            skip_ws();
        }

        return JsonValue(std::move(array));
    }

    std::string parse_string() {
        expect('"');
        std::ostringstream out;

        while (index_ < source_.size()) {
            const char c = source_[index_++];
            if (c == '"') {
                return out.str();
            }

            if (c != '\\') {
                out << c;
                continue;
            }

            if (index_ >= source_.size()) {
                throw std::runtime_error("Unterminated escape in JSON string.");
            }

            const char esc = source_[index_++];
            switch (esc) {
                case '"': out << '"'; break;
                case '\\': out << '\\'; break;
                case '/': out << '/'; break;
                case 'b': out << '\b'; break;
                case 'f': out << '\f'; break;
                case 'n': out << '\n'; break;
                case 'r': out << '\r'; break;
                case 't': out << '\t'; break;
                case 'u': {
                    if (index_ + 4 > source_.size()) {
                        throw std::runtime_error("Invalid unicode escape in JSON string.");
                    }
                    unsigned int code = 0;
                    for (int i = 0; i < 4; ++i) {
                        code = (code << 4) | static_cast<unsigned int>(hex_to_char(source_[index_++]));
                    }
                    if (code <= 0x7F) {
                        out << static_cast<char>(code);
                    } else if (code <= 0x7FF) {
                        out << static_cast<char>(0xC0 | ((code >> 6) & 0x1F));
                        out << static_cast<char>(0x80 | (code & 0x3F));
                    } else {
                        out << static_cast<char>(0xE0 | ((code >> 12) & 0x0F));
                        out << static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                        out << static_cast<char>(0x80 | (code & 0x3F));
                    }
                    break;
                }
                default:
                    throw std::runtime_error("Invalid escape sequence in JSON string.");
            }
        }

        throw std::runtime_error("Unterminated JSON string.");
    }

    double parse_number() {
        const std::size_t start = index_;
        if (source_[index_] == '-') {
            ++index_;
        }
        while (index_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[index_])) != 0) {
            ++index_;
        }
        if (index_ < source_.size() && source_[index_] == '.') {
            ++index_;
            while (index_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[index_])) != 0) {
                ++index_;
            }
        }
        if (index_ < source_.size() && (source_[index_] == 'e' || source_[index_] == 'E')) {
            ++index_;
            if (index_ < source_.size() && (source_[index_] == '+' || source_[index_] == '-')) {
                ++index_;
            }
            while (index_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[index_])) != 0) {
                ++index_;
            }
        }

        return std::stod(source_.substr(start, index_ - start));
    }
};

const JsonValue::Object &expect_object(const JsonValue &value, const std::string &context) {
    if (value.type() != JsonValue::Type::Object) {
        throw std::runtime_error(context + " must be an object.");
    }
    return value.as_object();
}

const JsonValue::Array &expect_array(const JsonValue &value, const std::string &context) {
    if (value.type() != JsonValue::Type::Array) {
        throw std::runtime_error(context + " must be an array.");
    }
    return value.as_array();
}

const std::string &expect_string(const JsonValue &value, const std::string &context) {
    if (value.type() != JsonValue::Type::String) {
        throw std::runtime_error(context + " must be a string.");
    }
    return value.as_string();
}

double expect_number(const JsonValue &value, const std::string &context) {
    if (value.type() != JsonValue::Type::Number) {
        throw std::runtime_error(context + " must be a number.");
    }
    return value.as_number();
}

const JsonValue &expect_key(const JsonValue::Object &object, const std::string &key, const std::string &context) {
    const auto it = object.find(key);
    if (it == object.end()) {
        throw std::runtime_error("Missing key '" + key + "' in " + context + ".");
    }
    return it->second;
}

std::optional<const JsonValue *> find_key(const JsonValue::Object &object, const std::string &key) {
    const auto it = object.find(key);
    if (it == object.end()) {
        return std::nullopt;
    }
    return &it->second;
}

class AstDecoder {
public:
    std::vector<StmtPtr> decode_stmt_array(const JsonValue &value) {
        std::vector<StmtPtr> stmts;
        for (const auto &entry : expect_array(value, "stmt array")) {
            stmts.push_back(decode_stmt(entry));
        }
        return stmts;
    }

    StmtPtr decode_stmt(const JsonValue &value) {
        if (value.type() == JsonValue::Type::Null) {
            return nullptr;
        }

        const auto &object = expect_object(value, "stmt");
        const std::string &type = expect_string(expect_key(object, "type", "stmt"), "stmt.type");

        if (type == "BlockStmt") {
            return std::make_shared<BlockStmt>(decode_stmt_array(expect_key(object, "statements", "BlockStmt")));
        }
        if (type == "ClassStmt") {
            std::vector<std::shared_ptr<FunctionStmt>> methods;
            for (const auto &method_value : expect_array(expect_key(object, "methods", "ClassStmt"), "ClassStmt.methods")) {
                const auto method_stmt = std::dynamic_pointer_cast<FunctionStmt>(decode_stmt(method_value));
                if (method_stmt == nullptr) {
                    throw std::runtime_error("ClassStmt.methods must contain FunctionStmt nodes.");
                }
                methods.push_back(method_stmt);
            }
            return std::make_shared<ClassStmt>(
                decode_token(expect_key(object, "name", "ClassStmt")),
                decode_expr(expect_key(object, "super", "ClassStmt")),
                methods,
                false,
                0
            );
        }
        if (type == "ExpressionStmt") {
            return std::make_shared<ExpressionStmt>(decode_expr(expect_key(object, "expression", "ExpressionStmt")));
        }
        if (type == "FunctionStmt") {
            const Token name = decode_token(expect_key(object, "name", "FunctionStmt"));
            const auto params = decode_params(expect_key(object, "params", "FunctionStmt"));
            auto blocks = decode_optional_blocks(expect_key(object, "blocks", "FunctionStmt"));
            auto body = decode_stmt_array(expect_key(object, "body", "FunctionStmt"));
            if (blocks.has_value()) {
                return std::make_shared<FunctionStmt>(name, params, blocks.value(), body, false);
            }
            return std::make_shared<FunctionStmt>(name, params, body, false);
        }
        if (type == "IfStmt") {
            return std::make_shared<IfStmt>(
                decode_expr(expect_key(object, "condition", "IfStmt")),
                decode_stmt(expect_key(object, "then", "IfStmt")),
                decode_stmt(expect_key(object, "else", "IfStmt"))
            );
        }
        if (type == "LogStmt") {
            return std::make_shared<LogStmt>(decode_expr(expect_key(object, "expression", "LogStmt")));
        }
        if (type == "ReturnStmt") {
            return std::make_shared<ReturnStmt>(
                decode_token(expect_key(object, "keyword", "ReturnStmt")),
                decode_expr_array(expect_key(object, "values", "ReturnStmt"))
            );
        }
        if (type == "LetStmt") {
            return std::make_shared<LetStmt>(
                decode_token(expect_key(object, "keyword", "LetStmt")),
                decode_token_array(expect_key(object, "names", "LetStmt")),
                decode_expr_array(expect_key(object, "inits", "LetStmt"))
            );
        }
        if (type == "WhileStmt") {
            return std::make_shared<WhileStmt>(
                decode_expr(expect_key(object, "condition", "WhileStmt")),
                decode_stmt(expect_key(object, "body", "WhileStmt")),
                decode_stmt(expect_key(object, "finally", "WhileStmt"))
            );
        }
        if (type == "ForStmt") {
            return std::make_shared<ForStmt>(
                decode_stmt(expect_key(object, "init", "ForStmt")),
                decode_expr(expect_key(object, "condition", "ForStmt")),
                decode_expr(expect_key(object, "inc", "ForStmt")),
                decode_stmt(expect_key(object, "body", "ForStmt")),
                decode_stmt(expect_key(object, "finally", "ForStmt"))
            );
        }
        if (type == "DoWhileStmt") {
            return std::make_shared<DoWhileStmt>(
                decode_stmt(expect_key(object, "body", "DoWhileStmt")),
                decode_expr(expect_key(object, "condition", "DoWhileStmt")),
                decode_stmt(expect_key(object, "finally", "DoWhileStmt"))
            );
        }
        if (type == "BreakStmt") {
            return std::make_shared<BreakStmt>(decode_token(expect_key(object, "keyword", "BreakStmt")));
        }
        if (type == "ContinueStmt") {
            return std::make_shared<ContinueStmt>(decode_token(expect_key(object, "keyword", "ContinueStmt")));
        }
        if (type == "DefStmt") {
            const Token name = decode_token(expect_key(object, "name", "DefStmt"));
            const auto params = decode_params(expect_key(object, "params", "DefStmt"));
            auto blocks = decode_optional_blocks(expect_key(object, "blocks", "DefStmt"));
            auto body = decode_stmt_array(expect_key(object, "body", "DefStmt"));
            if (blocks.has_value()) {
                return std::make_shared<DefStmt>(name, params, blocks.value(), body, false);
            }
            return std::make_shared<DefStmt>(name, params, body, false);
        }
        if (type == "ImportStmt") {
            return std::make_shared<ImportStmt>(
                decode_token(expect_key(object, "keyword", "ImportStmt")),
                decode_token(expect_key(object, "path", "ImportStmt")),
                decode_token(expect_key(object, "alias", "ImportStmt"))
            );
        }
        if (type == "FromImportStmt") {
            return std::make_shared<FromImportStmt>(
                decode_token(expect_key(object, "keyword", "FromImportStmt")),
                decode_token(expect_key(object, "path", "FromImportStmt")),
                decode_import_names(expect_key(object, "names", "FromImportStmt"))
            );
        }
        if (type == "ExportStmt") {
            return std::make_shared<ExportStmt>(decode_stmt(expect_key(object, "declaration", "ExportStmt")));
        }

        throw std::runtime_error("Unknown stmt type '" + type + "'.");
    }

    ExprPtr decode_expr(const JsonValue &value) {
        if (value.type() == JsonValue::Type::Null) {
            return nullptr;
        }

        const auto &object = expect_object(value, "expr");
        const std::string &type = expect_string(expect_key(object, "type", "expr"), "expr.type");

        ExprPtr expr;
        if (type == "AssignExpr") {
            expr = std::make_shared<AssignExpr>(
                decode_token(expect_key(object, "name", "AssignExpr")),
                decode_expr(expect_key(object, "value", "AssignExpr"))
            );
        } else if (type == "BinaryExpr") {
            expr = std::make_shared<BinaryExpr>(
                decode_expr(expect_key(object, "left", "BinaryExpr")),
                decode_token(expect_key(object, "op", "BinaryExpr")),
                decode_expr(expect_key(object, "right", "BinaryExpr"))
            );
        } else if (type == "CallExpr") {
            expr = std::make_shared<CallExpr>(
                decode_expr(expect_key(object, "callee", "CallExpr")),
                decode_token(expect_key(object, "paren", "CallExpr")),
                decode_expr_array(expect_key(object, "arguments", "CallExpr"))
            );
        } else if (type == "FunctionExpr") {
            expr = std::make_shared<FunctionExpr>(decode_stmt_array(expect_key(object, "body", "FunctionExpr")));
        } else if (type == "IndexExpr") {
            expr = std::make_shared<IndexExpr>(
                decode_expr(expect_key(object, "obj", "IndexExpr")),
                decode_token(expect_key(object, "bracket", "IndexExpr")),
                decode_expr(expect_key(object, "key", "IndexExpr"))
            );
        } else if (type == "ListExpr") {
            expr = std::make_shared<ListExpr>(decode_expr_array(expect_key(object, "elements", "ListExpr")));
        } else if (type == "NamedBlockExpr") {
            expr = std::make_shared<NamedBlockExpr>(
                decode_token(expect_key(object, "name", "NamedBlockExpr")),
                decode_expr(expect_key(object, "value", "NamedBlockExpr"))
            );
        } else if (type == "GetExpr") {
            expr = std::make_shared<GetExpr>(
                decode_expr(expect_key(object, "obj", "GetExpr")),
                decode_token(expect_key(object, "name", "GetExpr"))
            );
        } else if (type == "GroupingExpr") {
            expr = std::make_shared<GroupingExpr>(decode_expr(expect_key(object, "expression", "GroupingExpr")));
        } else if (type == "LiteralExpr") {
            expr = std::make_shared<LiteralExpr>(decode_literal_value(expect_key(object, "value", "LiteralExpr")));
        } else if (type == "LogicalExpr") {
            expr = std::make_shared<LogicalExpr>(
                decode_expr(expect_key(object, "left", "LogicalExpr")),
                decode_token(expect_key(object, "op", "LogicalExpr")),
                decode_expr(expect_key(object, "right", "LogicalExpr"))
            );
        } else if (type == "SetExpr") {
            expr = std::make_shared<SetExpr>(
                decode_expr(expect_key(object, "obj", "SetExpr")),
                decode_token(expect_key(object, "name", "SetExpr")),
                decode_expr(expect_key(object, "value", "SetExpr"))
            );
        } else if (type == "SuperExpr") {
            expr = std::make_shared<SuperExpr>(
                decode_token(expect_key(object, "keyword", "SuperExpr")),
                decode_token(expect_key(object, "method", "SuperExpr"))
            );
        } else if (type == "SelfExpr") {
            expr = std::make_shared<SelfExpr>(decode_token(expect_key(object, "keyword", "SelfExpr")));
        } else if (type == "UnaryExpr") {
            expr = std::make_shared<UnaryExpr>(
                decode_token(expect_key(object, "op", "UnaryExpr")),
                decode_expr(expect_key(object, "right", "UnaryExpr"))
            );
        } else if (type == "UpdateExpr") {
            const auto &is_prefix_value = expect_key(object, "isPrefix", "UpdateExpr");
            if (is_prefix_value.type() != JsonValue::Type::Bool) {
                throw std::runtime_error("UpdateExpr.isPrefix must be boolean.");
            }
            expr = std::make_shared<UpdateExpr>(
                decode_expr(expect_key(object, "target", "UpdateExpr")),
                decode_token(expect_key(object, "op", "UpdateExpr")),
                is_prefix_value.as_bool()
            );
        } else if (type == "VariableExpr") {
            expr = std::make_shared<VariableExpr>(decode_token(expect_key(object, "name", "VariableExpr")));
        } else {
            throw std::runtime_error("Unknown expr type '" + type + "'.");
        }

        if (const auto id_value = find_key(object, "id"); id_value.has_value()) {
            const std::size_t id = static_cast<std::size_t>(expect_number(*id_value.value(), "expr.id"));
            expr_by_id_.insert_or_assign(id, expr.get());
        }

        return expr;
    }

    const std::unordered_map<std::size_t, const Expr *> &expr_ids() const {
        return expr_by_id_;
    }

private:
    std::unordered_map<std::size_t, const Expr *> expr_by_id_;

    static Token decode_token(const JsonValue &value) {
        const auto &object = expect_object(value, "token");
        Token token(
            static_cast<TokenType>(static_cast<int>(expect_number(expect_key(object, "type", "token"), "token.type"))),
            expect_string(expect_key(object, "lexeme", "token"), "token.lexeme"),
            FilePos{
                static_cast<std::size_t>(expect_number(expect_key(object, "line", "token"), "token.line")),
                static_cast<std::size_t>(expect_number(expect_key(object, "column", "token"), "token.column")),
                ""
            }
        );
        return token;
    }

    std::vector<ExprPtr> decode_expr_array(const JsonValue &value) {
        std::vector<ExprPtr> exprs;
        for (const auto &entry : expect_array(value, "expr array")) {
            exprs.push_back(decode_expr(entry));
        }
        return exprs;
    }

    std::vector<Token> decode_token_array(const JsonValue &value) {
        std::vector<Token> tokens;
        for (const auto &entry : expect_array(value, "token array")) {
            tokens.push_back(decode_token(entry));
        }
        return tokens;
    }

    static std::vector<FunctionStmt::Parameter> decode_params(const JsonValue &value) {
        std::vector<FunctionStmt::Parameter> params;
        for (const auto &entry : expect_array(value, "params")) {
            const auto &object = expect_object(entry, "param");
            const Token name = decode_token(expect_key(object, "name", "param"));
            const JsonValue &is_variadic_value = expect_key(object, "isVariadic", "param");
            if (is_variadic_value.type() != JsonValue::Type::Bool) {
                throw std::runtime_error("param.isVariadic must be boolean.");
            }
            params.push_back(is_variadic_value.as_bool() ? FunctionStmt::Parameter::variadic(name) : FunctionStmt::Parameter::regular(name));
        }
        return params;
    }

    static std::optional<std::vector<BlockLiteral>> decode_optional_blocks(const JsonValue &value) {
        if (value.type() == JsonValue::Type::Null) {
            return std::nullopt;
        }

        std::vector<BlockLiteral> blocks;
        for (const auto &entry : expect_array(value, "blocks")) {
            const auto &object = expect_object(entry, "block literal");
            const Token name = decode_token(expect_key(object, "name", "block literal"));
            const JsonValue &expect_value = expect_key(object, "expect", "block literal");
            if (expect_value.type() == JsonValue::Type::Null) {
                blocks.emplace_back(name);
            } else {
                blocks.emplace_back(name, decode_token(expect_value));
            }
        }
        return blocks;
    }

    static std::vector<ImportName> decode_import_names(const JsonValue &value) {
        std::vector<ImportName> names;
        for (const auto &entry : expect_array(value, "import names")) {
            const auto &object = expect_object(entry, "import name");
            names.push_back(ImportName{
                decode_token(expect_key(object, "name", "import name")),
                decode_token(expect_key(object, "alias", "import name"))
            });
        }
        return names;
    }

    static Value decode_literal_value(const JsonValue &value) {
        const auto &object = expect_object(value, "literal value");
        const std::string &type = expect_string(expect_key(object, "type", "literal value"), "literal type");

        if (type == "None") {
            return Value::none();
        }
        if (type == "Boolean") {
            const JsonValue &bool_value = expect_key(object, "value", "boolean literal");
            if (bool_value.type() != JsonValue::Type::Bool) {
                throw std::runtime_error("Boolean literal value must be bool.");
            }
            return Value::boolean(bool_value.as_bool());
        }
        if (type == "Number") {
            return Value::number(expect_number(expect_key(object, "value", "number literal"), "number literal value"));
        }
        if (type == "String") {
            return Value::object(std::make_shared<ObjString>(expect_string(expect_key(object, "value", "string literal"), "string literal value")));
        }

        throw std::runtime_error("Unsupported literal type '" + type + "'.");
    }
};

} // namespace

bool load_artifact_program(const std::string &artifact_path, ArtifactProgram &out_program, std::string &error) {
    try {
        std::ifstream in(artifact_path);
        if (!in.is_open()) {
            error = "Could not open artifact '" + artifact_path + "'.";
            return false;
        }

        std::ostringstream buffer;
        buffer << in.rdbuf();
        JsonParser parser(buffer.str());
        const JsonValue root = parser.parse();
        const auto &root_object = expect_object(root, "artifact root");

        out_program.schema = expect_string(expect_key(root_object, "schema", "artifact"), "artifact.schema");
        out_program.version = expect_string(expect_key(root_object, "version", "artifact"), "artifact.version");
        out_program.entry_module = expect_string(expect_key(root_object, "entryModule", "artifact"), "artifact.entryModule");

        out_program.modules.clear();
        for (const auto &module_value : expect_array(expect_key(root_object, "modules", "artifact"), "artifact.modules")) {
            const auto &module_object = expect_object(module_value, "artifact module");
            const std::string module_id = expect_string(expect_key(module_object, "id", "module"), "module.id");

            AstDecoder decoder;
            ArtifactModule module;
            module.ast = decoder.decode_stmt_array(expect_key(module_object, "ast", "module"));

            if (const auto path_value = find_key(module_object, "path"); path_value.has_value()) {
                module.source_path = expect_string(*path_value.value(), "module.path");
            }

            if (const auto import_map_value = find_key(module_object, "importMap"); import_map_value.has_value()) {
                for (const auto &mapping_value : expect_array(*import_map_value.value(), "module.importMap")) {
                    const auto &mapping_object = expect_object(mapping_value, "module import mapping");
                    const std::string raw = expect_string(expect_key(mapping_object, "raw", "module import mapping"), "module import raw");
                    const std::string target = expect_string(expect_key(mapping_object, "module", "module import mapping"), "module import target");
                    module.import_map.insert_or_assign(raw, target);
                }
            }

            if (const auto resolved_locals_value = find_key(module_object, "resolvedLocals"); resolved_locals_value.has_value()) {
                for (const auto &entry_value : expect_array(*resolved_locals_value.value(), "module.resolvedLocals")) {
                    const auto &entry_object = expect_object(entry_value, "resolved local entry");
                    const std::size_t expr_id = static_cast<std::size_t>(expect_number(expect_key(entry_object, "exprId", "resolved local entry"), "resolved local exprId"));
                    const int depth = static_cast<int>(expect_number(expect_key(entry_object, "depth", "resolved local entry"), "resolved local depth"));

                    const auto it = decoder.expr_ids().find(expr_id);
                    if (it == decoder.expr_ids().end()) {
                        throw std::runtime_error("resolvedLocals references unknown exprId " + std::to_string(expr_id) + ".");
                    }
                    module.resolved_locals.insert_or_assign(it->second, depth);
                }
            }

            out_program.modules.insert_or_assign(module_id, std::move(module));
        }

        if (out_program.modules.find(out_program.entry_module) == out_program.modules.end()) {
            error = "Artifact entry module '" + out_program.entry_module + "' is missing from module list.";
            return false;
        }

        return true;
    } catch (const std::exception &ex) {
        error = std::string("Failed to parse artifact: ") + ex.what();
        return false;
    }
}