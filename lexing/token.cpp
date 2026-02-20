//
// Created by sullivanb on 2/20/26.
//

#include "token.h"

#include <charconv>
#include <optional>

void FilePosFactory::move(const char c) {
    switch (c) {
        case '\n':
            ++current.line_no;
            current.column_no = 1;
            break;

        default:
            ++current.column_no;
            break;
    }
}

Token::Token(const TokenType type, const FilePos pos)
    : type{type}, literal{"", 0.0}, file_pos{pos} {}

Token::Token(const TokenType type, const std::string text, const FilePos pos)
    : type{type}, literal{text, 0.0}, file_pos{pos} {}

Token::Token(const double number, const FilePos pos)
    : type{TokenType::Number}, literal{"", number}, file_pos{pos} {}

void TokenFactory::feed(const char c) {
    builder << c;
    file_pos_factory.move(c);
}

struct SvDoubleResult {
    double value{};
    std::optional<std::string> err;
};

SvDoubleResult convert_sv_to_double(const std::string &sv) {
    double value;
    // Pointers to the start and end of the string_view's character sequence
    const char* start = sv.data();
    const char* end = start + sv.size();

    if (auto [ptr, ec] = std::from_chars(start, end, value); ec == std::errc{}) {
        return {value, std::nullopt};
    } else if (ec == std::errc::invalid_argument) {
        return {0.0,"Error: Invalid argument (not a valid number)"};
    } else if (ec == std::errc::result_out_of_range) {
        return {0.0,"Error: Result out of range for double"};
    } else {
        return {0.0, "Error: Unknown conversion error"};
    }
}

Token TokenFactory::compile(const TokenType type) {
    const std::string lexeme = builder.str();

    builder.str("");
    builder.clear();

    FilePos file_pos = file_pos_factory.current;
    file_pos.column_no -= lexeme.length(); // offset with length of token

    switch (type) {
        case TokenType::Number: {
            if (auto [value, err] = convert_sv_to_double(lexeme); err == std::nullopt) {
                return Token{value, file_pos};
            } else {
                return Token{TokenType::Illegal, err.value(), file_pos};
            }
        }

        case TokenType::String:
        case TokenType::Identifier:
        case TokenType::ExprIdentifier:
        case TokenType::BlockIdentifier:
            return Token{type, lexeme, file_pos};

        default:
            return Token{type, file_pos};
    }
}
