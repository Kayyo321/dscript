//
// Created by sullivanb on 2/20/26.
//

#include "lexer.h"

#include <utility>

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens{};

    for (Token t{next_token()}; t.type != TokenType::EndOfInput; t = next_token()) {
        if (t.type == TokenType::Illegal)
            had_error = true;

        tokens.push_back(t);
    }

    return tokens;
}

Token Lexer::next_token() {
    skip_whitespace();

    if (is_at_end()) {
        return token_factory.compile(TokenType::EndOfInput);
    }

    const char c = peek(0);
    if ((isalpha(c) || c == '_') || (c == '$' || c == '%' && isalpha(peek(1)))) {
        return identifier(c);
    } else if ((isdigit(c)) || (c == '.' && isdigit(peek(1)))) {
        return number();
    } else if (c == '\'' || c == '"') {
        return string();
    } else {
        return symbol();
    }
}

Token Lexer::string() {
    const char symbol = peek(0);
    next(false); // don't add " (or ') to the buffer

    while (!is_at_end()) {
        if (peek(0) == symbol) {
            break;
        }

        if (peek(0) == '\\') {
            next(true);
            if (!is_at_end()) {
                next(true);
            }
            continue;
        }

        next(true);
    }

    next(false); // skip ending " (or ') but don't add it to the buffer

    return token_factory.compile(TokenType::String);
}

Token Lexer::number() {
    bool saw_period_already = peek(0) == '.';

    while (isdigit(peek(0)) || (!saw_period_already && peek(0) == '.')) {
        if (!saw_period_already && peek(0) == '.') {
            saw_period_already = true;
        }

        next(true);
    }

    return token_factory.compile(TokenType::Number);
}

Token Lexer::identifier(const char start) {
    if (start == '$' || start == '%') {
        next(true);
    }

    while (isalnum(peek(0)) || peek(0) == '_') {
        next(true);
    }

    switch (start) {
        case '$':
            return token_factory.compile(TokenType::BlockIdentifier);

        case '%':
            return token_factory.compile(TokenType::ExprIdentifier);

        default:
            break;
    }

    Token id_or_keyword = token_factory.compile(TokenType::Identifier);
    if (keywords.find(id_or_keyword.literal.lexeme) != keywords.end()) {
        id_or_keyword.type = keywords.at(id_or_keyword.literal.lexeme);
    }

    return id_or_keyword;
}

Token Lexer::symbol() {
    switch (peek(0)) {
        case '+':
            if (peek(1) == '=') return eat_then_compile(TokenType::PlusEquals, 2);
            if (peek(1) == '+') return eat_then_compile(TokenType::PlusPlus, 2);
            return eat_then_compile(TokenType::Plus);

        case '-':
            if (peek(1) == '=') return eat_then_compile(TokenType::MinusEquals, 2);
            if (peek(1) == '-') return eat_then_compile(TokenType::MinusMinus, 2);
            return eat_then_compile(TokenType::Minus);

        case '*':
            if (peek(1) == '=') return eat_then_compile(TokenType::StarEquals, 2);
            return eat_then_compile(TokenType::Star);

        case '/':
            if (peek(1) == '=') return eat_then_compile(TokenType::SlashEquals, 2);
            return eat_then_compile(TokenType::Slash);

        case '%':
            if (peek(1) == '=') return eat_then_compile(TokenType::ModuloEquals, 2);
            return eat_then_compile(TokenType::Modulo);

        case '^':
            if (peek(1) == '=') return eat_then_compile(TokenType::PowerEquals, 2);
            return eat_then_compile(TokenType::Power);

        case '.': 
            if (peek(1) == '.' && peek(2) == '.') return eat_then_compile(TokenType::Ellipse, 3);
            return eat_then_compile(TokenType::Period);

        case ',': return eat_then_compile(TokenType::Comma);
        case ':': return eat_then_compile(TokenType::Colon);
        case ';': return eat_then_compile(TokenType::Semicolon);
        case '(': return eat_then_compile(TokenType::LeftParen);
        case ')': return eat_then_compile(TokenType::RightParen);
        case '[': return eat_then_compile(TokenType::LeftBracket);
        case ']': return eat_then_compile(TokenType::RightBracket);
        case '{': return eat_then_compile(TokenType::LeftBrace);
        case '}': return eat_then_compile(TokenType::RightBrace);

        case '=':
            if (peek(1) == '=') return eat_then_compile(TokenType::Is, 2);
            else return eat_then_compile(TokenType::Equals);

        case '!':
            if (peek(1) == '=') return eat_then_compile(TokenType::Not, 2);
            return eat_then_compile(TokenType::Not);

        case '>':
            if (peek(1) == '=') return eat_then_compile(TokenType::GreaterEqual, 2);
            else return eat_then_compile(TokenType::GreaterThan);

        case '<':
            if (peek(1) == '=') return eat_then_compile(TokenType::LessEqual, 2);
            else return eat_then_compile(TokenType::LessThan);

        default:
            return eat_then_compile(TokenType::Illegal);
    }
}

void Lexer::skip_whitespace() {
    while (!is_at_end()) {
        if (isspace(peek(0))) {
            next(false);
            continue;
        }

        if (peek(0) == '/' && peek(1) == '/') {
            while (!is_at_end() && peek(0) != '\n') {
                next(false);
            }
            continue;
        }

        if (peek(0) == '/' && peek(1) == '*') {
            next(false);
            next(false);

            bool terminated = false;
            while (!is_at_end()) {
                if (peek(0) == '*' && peek(1) == '/') {
                    next(false);
                    next(false);
                    terminated = true;
                    break;
                }

                next(false);
            }

            if (!terminated) {
                had_error = true;
            }

            continue;
        }

        break;
    }
}

bool Lexer::is_at_end() {
    return peek(0) == '\0';
}

Token Lexer::eat_then_compile(const TokenType type) { // ease of use without a for-loop
    next(true);
    return token_factory.compile(type);
}

Token Lexer::eat_then_compile(const TokenType type, const std::size_t bytes) {
    for (std::size_t i = 0; i < bytes; ++i) {
        next(true);
    }

    return token_factory.compile(type);
}

StringLexer::StringLexer(std::string sv)
    : text{std::move(sv)}, iter{text.begin()} {
    token_factory.file_pos_factory.current.path = "<string>";
}

char StringLexer::peek(const int offset) {
    if (offset > 0 && (iter + offset) > text.end()) {
        return '\0';
    } else if (offset < 0 && (iter - offset) < text.begin()) {
        return text[0];
    } else {
        return *(iter + offset);
    }
}

char StringLexer::next(const bool feed) {
    if (iter == text.end() || *iter == '\0') {
        return '\0';
    }

    const char c = *(iter++);

    if (feed) {
        token_factory.feed(c);
    } else {
        token_factory.file_pos_factory.move(c); // only move file-pos, don't add char to buffer
    }

    return c;
}

FileLexer::FileLexer(const std::string &path)
    : file{std::ifstream(path)} {
    token_factory.file_pos_factory.current.path = path;
}

char FileLexer::peek(const int offset) {
    const std::streampos current = file.tellg();
    if (current == std::streampos(-1))
        return '\0';

    const std::streampos target = current + static_cast<std::streamoff>(offset);
    if (target < std::streampos(0))
        return '\0';

    file.seekg(target);
    const int c = file.peek();
    file.seekg(current);

    if (c == EOF || c == '\0') {
        return '\0';
    }

    return static_cast<char>(c);
}

char FileLexer::next(const bool feed) {
    char c;
    if (!file.get(c))
        return '\0';

    if (feed) {
        token_factory.feed(c);
    } else {
        token_factory.file_pos_factory.move(c);
    }

    return c;
}
