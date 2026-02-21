//
// Created by sullivanb on 2/20/26.
//

#include "token.h"

#include <charconv>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>

static bool is_hex_digit(const char c) {
    return std::isxdigit(static_cast<unsigned char>(c)) != 0;
}

static int hex_value(const char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static bool append_utf8(std::string &out, const uint32_t codepoint) {
    if (codepoint > 0x10FFFF) {
        return false;
    }

    if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
        return false;
    }

    if (codepoint <= 0x7F) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }

    return true;
}

static std::optional<std::string> decode_escapes(const std::string &input, std::optional<std::string> &err) {
    std::string out;
    out.reserve(input.size());

    for (std::size_t i = 0; i < input.size(); ++i) {
        const char c = input[i];
        if (c != '\\') {
            out.push_back(c);
            continue;
        }

        if (i + 1 >= input.size()) {
            err = "Error: Trailing backslash in string literal";
            return std::nullopt;
        }

        const char esc = input[++i];
        switch (esc) {
            case '\\': out.push_back('\\'); break;
            case '\'': out.push_back('\''); break;
            case '"': out.push_back('"'); break;
            case '?': out.push_back('?'); break;
            case 'a': out.push_back('\a'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'v': out.push_back('\v'); break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7': {
                int value = esc - '0';
                int consumed = 0;
                while (consumed < 2 && (i + 1) < input.size() && input[i + 1] >= '0' && input[i + 1] <= '7') {
                    value = (value * 8) + (input[i + 1] - '0');
                    ++i;
                    ++consumed;
                }
                out.push_back(static_cast<char>(value & 0xFF));
                break;
            }
            case 'x': {
                if (i + 2 >= input.size() || !is_hex_digit(input[i + 1]) || !is_hex_digit(input[i + 2])) {
                    err = "Error: Invalid \\x escape (expected two hex digits)";
                    return std::nullopt;
                }

                const int value = (hex_value(input[i + 1]) << 4) | hex_value(input[i + 2]);
                out.push_back(static_cast<char>(value));
                i += 2;
                break;
            }
            case 'u':
            case 'U': {
                const int digits = esc == 'u' ? 4 : 8;
                if (i + digits >= input.size()) {
                    err = "Error: Incomplete unicode escape";
                    return std::nullopt;
                }

                uint32_t codepoint = 0;
                for (int d = 1; d <= digits; ++d) {
                    const char h = input[i + d];
                    if (!is_hex_digit(h)) {
                        err = "Error: Invalid unicode escape digits";
                        return std::nullopt;
                    }
                    codepoint = (codepoint << 4) | static_cast<uint32_t>(hex_value(h));
                }
                i += digits;

                if (!append_utf8(out, codepoint)) {
                    err = "Error: Unicode escape out of range";
                    return std::nullopt;
                }

                break;
            }
            default:
                err = std::string("Error: Unknown escape sequence \\") + esc;
                return std::nullopt;
        }
    }

    return out;
}

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
            if (auto err = std::optional<std::string>{}; auto decoded = decode_escapes(lexeme, err)) {
                return Token{type, decoded.value(), file_pos};
            } else {
                return Token{TokenType::Illegal, err.value(), file_pos};
            }

        case TokenType::Identifier:
        case TokenType::ExprIdentifier:
        case TokenType::BlockIdentifier:
            return Token{type, lexeme, file_pos};

        default:
            return Token{type, file_pos};
    }
}
