//
// Created by sullivanb on 2/20/26.
//

#include "parser.h"

#include "expr.h"

#include "../runtime/objs.h"

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : tokens{std::move(tokens)} {}

    std::vector<StmtPtr> parse() {
        std::vector<StmtPtr> statements;

        while (!is_at_end()) {
            try {
                StmtPtr stmt = parse_stmt();
                if (stmt != nullptr) {
                    statements.push_back(std::move(stmt));
                }
            } catch (const ParseError &) {
                synchronize();
            }
        }

        return statements;
    }

private:
    struct EnumClassHash {
        template <typename T>
        std::size_t operator()(T t) const {
            return static_cast<std::size_t>(t);
        }
    };

    using StmtParser = StmtPtr (Parser::*)();

    static const std::unordered_map<TokenType, StmtParser, EnumClassHash> kStmtDispatch;

    struct ParseError : std::runtime_error {
        explicit ParseError(const std::string &message)
            : std::runtime_error(message) {}
    };

    StmtPtr parse_stmt() {
        if (!is_at_end()) {
            if (const auto it = kStmtDispatch.find(peek().type); it != kStmtDispatch.end()) {
                advance();
                return (this->*(it->second))();
            }
        }

        if (match(TokenType::LeftBrace)) {
            return std::make_shared<BlockStmt>(parse_block_statements());
        }

        return parse_expression_stmt();
    }

    bool is_at_end() const {
        return current >= tokens.size();
    }

    const Token &peek() const {
        return tokens[current];
    }

    const Token &previous() const {
        return tokens[current - 1];
    }

    const Token &advance() {
        if (!is_at_end()) {
            ++current;
        }
        return previous();
    }

    bool check(TokenType type) const {
        return !is_at_end() && peek().type == type;
    }

    bool check_next(TokenType type) const {
        return (current + 1 < tokens.size()) && tokens[current + 1].type == type;
    }

    bool check_next_next(TokenType type) const {
        return (current + 2 < tokens.size()) && tokens[current + 2].type == type;
    }

    bool match(TokenType type) {
        if (!check(type)) {
            return false;
        }
        advance();
        return true;
    }

    Token consume_or_throw(TokenType type, const std::string &message) {
        if (check(type)) {
            return advance();
        }
        throw ParseError(message);
    }

    Token consume(TokenType type, const Token &fallback) {
        if (check(type)) {
            return advance();
        }
        return fallback;
    }

    bool matches_any(std::initializer_list<TokenType> types) {
        for (TokenType type : types) {
            if (match(type)) {
                return true;
            }
        }
        return false;
    }

    void synchronize() {
        while (!is_at_end()) {
            if (match(TokenType::Semicolon)) {
                return;
            }

            switch (peek().type) {
                case TokenType::Class:
                case TokenType::Fn:
                case TokenType::Def:
                case TokenType::If:
                case TokenType::Do:
                case TokenType::For:
                case TokenType::While:
                case TokenType::Log:
                case TokenType::Let:
                case TokenType::Return:
                    return;
                default:
                    break;
            }

            advance();
        }
    }

    std::vector<StmtPtr> parse_block_statements() {
        std::vector<StmtPtr> statements;
        while (!is_at_end() && !check(TokenType::RightBrace)) {
            StmtPtr stmt = parse_stmt();
            if (stmt != nullptr) {
                statements.push_back(std::move(stmt));
            }
        }
        consume_or_throw(TokenType::RightBrace, "Expected '}' after block.");
        return statements;
    }

    std::vector<Token> parse_params() {
        std::vector<Token> params;
        consume_or_throw(TokenType::LeftParen, "Expected '(' before parameter list.");

        while (!is_at_end() && !check(TokenType::RightParen)) {
            if (check(TokenType::Identifier) || check(TokenType::ExprIdentifier)) {
                params.push_back(advance());
            } else {
                throw ParseError("Expected parameter name.");
            }

            if (!match(TokenType::Comma)) {
                break;
            }
        }

        consume_or_throw(TokenType::RightParen, "Expected ')' after parameter list.");
        return params;
    }

    std::optional<std::vector<BlockLiteral>> parse_block_literals_if_present() {
        if (!check(TokenType::LeftParen)) {
            return std::nullopt;
        }

        if (!check_next(TokenType::BlockIdentifier)
            && !(check_next(TokenType::Identifier) && check_next_next(TokenType::Colon))) {
            return std::nullopt;
        }

        advance();

        std::vector<BlockLiteral> blocks;
        while (!is_at_end() && !check(TokenType::RightParen)) {
            if (check(TokenType::Identifier) && check_next(TokenType::Colon)) {
                Token expected_name = advance();
                consume_or_throw(TokenType::Colon, "Expected ':' after named block label.");
                Token block_name = consume_or_throw(TokenType::BlockIdentifier, "Expected block-literal parameter name after ':'.");
                blocks.emplace_back(block_name, expected_name);
            } else {
                Token block_name = consume_or_throw(TokenType::BlockIdentifier, "Expected block-literal parameter name.");

                if (match(TokenType::Colon)) {
                    if (check(TokenType::Identifier) || check(TokenType::ExprIdentifier) || check(TokenType::BlockIdentifier)) {
                        blocks.emplace_back(block_name, advance());
                    } else {
                        throw ParseError("Expected block-literal expectation after ':'.");
                    }
                } else {
                    blocks.emplace_back(block_name);
                }
            }

            if (!match(TokenType::Comma)) {
                break;
            }
        }

        consume_or_throw(TokenType::RightParen, "Expected ')' after block-literal parameters.");
        return blocks;
    }

    std::vector<StmtPtr> parse_decl_body() {
        if (match(TokenType::Semicolon)) {
            return {};
        }

        consume_or_throw(TokenType::LeftBrace, "Expected '{' before declaration body.");
        return parse_block_statements();
    }

    StmtPtr parse_class_stmt() {
        static const Token anonymous{TokenType::Identifier, "<anonymous-class>", FilePos{}};

        Token name = consume(TokenType::Identifier, anonymous);
        ExprPtr super_class = nullptr;
        if (match(TokenType::Is)) {
            Token super_name = consume_or_throw(TokenType::Identifier, "Expected superclass name after 'is'.");
            super_class = std::make_shared<VariableExpr>(super_name);
        }

        consume_or_throw(TokenType::LeftBrace, "Expected '{' before class body.");

        std::vector<std::shared_ptr<FunctionStmt>> methods;
        while (!is_at_end() && !check(TokenType::RightBrace)) {
            consume_or_throw(TokenType::Fn, "Expected 'fn' method declaration inside class.");
            auto method = std::dynamic_pointer_cast<FunctionStmt>(parse_fn_stmt());
            if (method == nullptr) {
                throw ParseError("Expected function statement in class body.");
            }
            methods.push_back(std::move(method));
        }

        consume_or_throw(TokenType::RightBrace, "Expected '}' after class body.");
        return std::make_shared<ClassStmt>(name, super_class, methods, false, 0);
    }

    StmtPtr parse_fn_stmt() {
        static const Token anonymous{TokenType::Identifier, "<anonymous-fn>", FilePos{}};

        Token name = consume(TokenType::Identifier, anonymous);
        const std::vector<Token> params = parse_params();
        const std::optional<std::vector<BlockLiteral>> blocks = parse_block_literals_if_present();
        const std::vector<StmtPtr> body = parse_decl_body();

        if (blocks.has_value()) {
            return std::make_shared<FunctionStmt>(name, params, blocks.value(), body, false);
        }
        return std::make_shared<FunctionStmt>(name, params, body, false);
    }

    StmtPtr parse_def_stmt() {
        static const Token anonymous{TokenType::Identifier, "<anonymous-def>", FilePos{}};

        Token name = consume(TokenType::Identifier, anonymous);

        std::vector<Token> params;
        if (check(TokenType::LeftParen)) {
            params = parse_params();
        }

        const std::optional<std::vector<BlockLiteral>> blocks = parse_block_literals_if_present();
        const std::vector<StmtPtr> body = parse_decl_body();

        if (blocks.has_value()) {
            return std::make_shared<DefStmt>(name, params, blocks.value(), body, false);
        }
        return std::make_shared<DefStmt>(name, params, body, false);
    }

    StmtPtr parse_if_stmt() {
        ExprPtr condition = parse_expression();
        if (match(TokenType::Then) || match(TokenType::Do)) {
        }

        StmtPtr then_branch = parse_stmt();
        StmtPtr else_branch = nullptr;
        if (match(TokenType::Else)) {
            else_branch = parse_stmt();
        }

        return std::make_shared<IfStmt>(condition, then_branch, else_branch);
    }

    StmtPtr parse_do_stmt() {
        StmtPtr body = parse_stmt();
        consume_or_throw(TokenType::While, "Expected 'while' after do-while body.");
        ExprPtr condition = parse_expression();
        match(TokenType::Semicolon);

        std::vector<StmtPtr> statements;
        statements.push_back(body);
        statements.push_back(std::make_shared<WhileStmt>(condition, body));
        return std::make_shared<BlockStmt>(statements);
    }

    StmtPtr parse_while_stmt() {
        ExprPtr condition = parse_expression();
        if (match(TokenType::Do)) {
        }
        StmtPtr body = parse_stmt();
        return std::make_shared<WhileStmt>(condition, body);
    }

    StmtPtr parse_for_stmt() {
        if (match(TokenType::LeftParen)) {
            StmtPtr initializer = nullptr;
            if (!check(TokenType::Semicolon)) {
                if (match(TokenType::Let)) {
                    initializer = parse_let_stmt();
                } else {
                    initializer = parse_expression_stmt();
                }
            } else {
                advance();
            }

            ExprPtr condition = nullptr;
            if (!check(TokenType::Semicolon)) {
                condition = parse_expression();
            }
            consume_or_throw(TokenType::Semicolon, "Expected ';' after loop condition.");

            ExprPtr increment = nullptr;
            if (!check(TokenType::RightParen)) {
                increment = parse_expression();
            }
            consume_or_throw(TokenType::RightParen, "Expected ')' after for clauses.");

            if (match(TokenType::Do)) {
            }
            StmtPtr body = parse_stmt();
            return std::make_shared<ForStmt>(initializer, condition, increment, body);
        }

        ExprPtr condition = parse_expression();
        if (match(TokenType::Do)) {
        }
        StmtPtr body = parse_stmt();
        return std::make_shared<ForStmt>(nullptr, condition, nullptr, body);
    }

    StmtPtr parse_log_stmt() {
        ExprPtr expression = parse_expression();
        match(TokenType::Semicolon);
        return std::make_shared<LogStmt>(expression);
    }

    StmtPtr parse_let_stmt() {
        const Token keyword = previous();
        Token name = consume_or_throw(TokenType::Identifier, "Expected variable name after 'let'.");
        ExprPtr init = nullptr;
        if (match(TokenType::Equals)) {
            init = parse_expression();
        }
        match(TokenType::Semicolon);
        return std::make_shared<LetStmt>(keyword, name, init);
    }

    StmtPtr parse_return_stmt() {
        const Token keyword = previous();
        ExprPtr value = nullptr;
        if (!check(TokenType::Semicolon)) {
            value = parse_expression();
        }
        match(TokenType::Semicolon);
        return std::make_shared<ReturnStmt>(keyword, value);
    }

    StmtPtr parse_expression_stmt() {
        ExprPtr expression = parse_expression();
        match(TokenType::Semicolon);
        return std::make_shared<ExpressionStmt>(expression);
    }

    ExprPtr parse_expression() {
        return parse_assignment();
    }

    ExprPtr parse_left_assoc(ExprPtr (Parser::*next)(), const std::initializer_list<TokenType> operators) {
        ExprPtr expr = (this->*next)();

        while (matches_any(operators)) {
            Token op = previous();
            ExprPtr right = (this->*next)();
            expr = std::make_shared<BinaryExpr>(expr, op, right);
        }

        return expr;
    }

    ExprPtr parse_assignment() {
        ExprPtr expr = parse_or();

        if (match(TokenType::Equals)) {
            ExprPtr value = parse_assignment();

            if (auto variable = std::dynamic_pointer_cast<VariableExpr>(expr)) {
                return std::make_shared<AssignExpr>(variable->name, value);
            }

            if (auto get = std::dynamic_pointer_cast<GetExpr>(expr)) {
                return std::make_shared<SetExpr>(get->obj, get->name, value);
            }

            throw ParseError("Invalid assignment target.");
        }

        return expr;
    }

    ExprPtr parse_or() {
        ExprPtr expr = parse_and();
        while (check(TokenType::Identifier) && peek().literal.lexeme == "or") {
            Token op = advance();
            ExprPtr right = parse_and();
            expr = std::make_shared<LogicalExpr>(expr, op, right);
        }
        return expr;
    }

    ExprPtr parse_and() {
        ExprPtr expr = parse_equality();
        while (match(TokenType::And)) {
            Token op = previous();
            ExprPtr right = parse_equality();
            expr = std::make_shared<LogicalExpr>(expr, op, right);
        }
        return expr;
    }

    ExprPtr parse_equality() {
        return parse_left_assoc(&Parser::parse_comparison, {TokenType::Is, TokenType::Not});
    }

    ExprPtr parse_comparison() {
        return parse_left_assoc(&Parser::parse_term, {TokenType::GreaterThan, TokenType::GreaterEqual, TokenType::LessThan, TokenType::LessEqual});
    }

    ExprPtr parse_term() {
        return parse_left_assoc(&Parser::parse_factor, {TokenType::Plus, TokenType::Minus});
    }

    ExprPtr parse_factor() {
        return parse_left_assoc(&Parser::parse_power, {TokenType::Star, TokenType::Slash, TokenType::Modulo});
    }

    ExprPtr parse_power() {
        return parse_left_assoc(&Parser::parse_unary, {TokenType::Power});
    }

    ExprPtr parse_unary() {
        if (matches_any({TokenType::Not, TokenType::Minus})) {
            Token op = previous();
            ExprPtr right = parse_unary();
            return std::make_shared<UnaryExpr>(op, right);
        }

        return parse_call();
    }

    ExprPtr parse_call_argument() {
        if (match(TokenType::Let)) {
            std::vector<StmtPtr> body;
            body.push_back(parse_let_stmt());
            return std::make_shared<FunctionExpr>(body);
        }

        if (check(TokenType::Identifier) && check_next(TokenType::Colon) && check_next_next(TokenType::LeftBrace)) {
            Token name = advance();
            consume_or_throw(TokenType::Colon, "Expected ':' after named block argument.");
            consume_or_throw(TokenType::LeftBrace, "Expected '{' after named block label.");
            return std::make_shared<NamedBlockExpr>(name, std::make_shared<FunctionExpr>(parse_block_statements()));
        }

        const bool previous_allow_implicit = allow_implicit_call;
        allow_implicit_call = false;
        ExprPtr argument = parse_expression();
        allow_implicit_call = previous_allow_implicit;
        return argument;
    }

    bool can_start_call_argument() const {
        return check(TokenType::Let)
            || check(TokenType::Identifier)
            || check(TokenType::ExprIdentifier)
            || check(TokenType::BlockIdentifier)
            || check(TokenType::Number)
            || check(TokenType::String)
            || check(TokenType::LeftParen)
            || check(TokenType::Minus)
            || check(TokenType::Not);
    }

    ExprPtr parse_call() {
        ExprPtr expr = parse_primary();

        while (true) {
            if (match(TokenType::LeftParen)) {
                std::vector<ExprPtr> args;
                if (!check(TokenType::RightParen)) {
                    do {
                        args.push_back(parse_call_argument());
                    } while (match(TokenType::Comma));
                }

                Token paren = consume_or_throw(TokenType::RightParen, "Expected ')' after arguments.");
                auto call_expr = std::make_shared<CallExpr>(expr, paren, args);

                if (match(TokenType::LeftBrace)) {
                    call_expr->arguments.push_back(std::make_shared<FunctionExpr>(parse_block_statements()));
                    expr = call_expr;
                    break;
                }

                expr = call_expr;
                continue;
            }

            const std::size_t expr_line = previous().file_pos.line_no;
            if (allow_implicit_call && can_start_call_argument() && peek().file_pos.line_no == expr_line) {
                std::vector<ExprPtr> args;
                args.push_back(parse_call_argument());

                while (match(TokenType::Comma)) {
                    args.push_back(parse_call_argument());
                }

                while (can_start_call_argument() && peek().file_pos.line_no == expr_line) {
                    args.push_back(parse_call_argument());
                }

                auto call_expr = std::make_shared<CallExpr>(expr, previous(), args);
                if (match(TokenType::LeftBrace)) {
                    call_expr->arguments.push_back(std::make_shared<FunctionExpr>(parse_block_statements()));
                    expr = call_expr;
                    break;
                }

                expr = call_expr;
                continue;
            }

            if (match(TokenType::Period)) {
                Token name = consume_or_throw(TokenType::Identifier, "Expected property name after '.'.");
                expr = std::make_shared<GetExpr>(expr, name);
                continue;
            }

            break;
        }

        return expr;
    }

    ExprPtr parse_primary() {
        if (match(TokenType::Number)) {
            return std::make_shared<LiteralExpr>(Value::number(previous().literal.number));
        }

        if (match(TokenType::String)) {
            return std::make_shared<LiteralExpr>(Value::object(std::make_shared<ObjString>(previous().literal.lexeme)));
        }

        if (match(TokenType::Identifier)) {
            if (previous().literal.lexeme == "true") {
                return std::make_shared<LiteralExpr>(Value::boolean(true));
            }

            if (previous().literal.lexeme == "false") {
                return std::make_shared<LiteralExpr>(Value::boolean(false));
            }

            if (previous().literal.lexeme == "none") {
                return std::make_shared<LiteralExpr>(Value::none());
            }

            if (previous().literal.lexeme == "self") {
                return std::make_shared<SelfExpr>(previous());
            }

            if (previous().literal.lexeme == "super") {
                const Token super_keyword = previous();
                if (match(TokenType::Period)) {
                    Token method = consume_or_throw(TokenType::Identifier, "Expected superclass method name after 'super.'.");
                    return std::make_shared<SuperExpr>(super_keyword, method);
                }
            }

            return std::make_shared<VariableExpr>(previous());
        }

        if (match(TokenType::ExprIdentifier)) {
            return std::make_shared<VariableExpr>(previous());
        }

        if (match(TokenType::BlockIdentifier)) {
            return std::make_shared<VariableExpr>(previous());
        }

        if (match(TokenType::LeftParen)) {
            ExprPtr expr = parse_expression();
            consume_or_throw(TokenType::RightParen, "Expected ')' after expression.");
            return std::make_shared<GroupingExpr>(expr);
        }

        throw ParseError("Expected expression.");
    }

    std::vector<Token> tokens{};
    std::size_t current{0};
    bool allow_implicit_call{true};
};

const std::unordered_map<TokenType, Parser::StmtParser, Parser::EnumClassHash> Parser::kStmtDispatch = {
    {TokenType::Class, &Parser::parse_class_stmt},
    {TokenType::Fn, &Parser::parse_fn_stmt},
    {TokenType::Def, &Parser::parse_def_stmt},
    {TokenType::If, &Parser::parse_if_stmt},
    {TokenType::Do, &Parser::parse_do_stmt},
    {TokenType::While, &Parser::parse_while_stmt},
    {TokenType::For, &Parser::parse_for_stmt},
    {TokenType::Log, &Parser::parse_log_stmt},
    {TokenType::Let, &Parser::parse_let_stmt},
    {TokenType::Return, &Parser::parse_return_stmt},
};

std::vector<StmtPtr> parse(Lexer &lexer) {
    return Parser{lexer.tokenize()}.parse();
}
