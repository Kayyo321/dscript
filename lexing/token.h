//
// Created by sullivanb on 2/20/26.
//

#ifndef DSCRIPT_TOKEN_H
#define DSCRIPT_TOKEN_H

#include <sstream>

enum class TokenType {
    EndOfInput,
    Illegal,

    Identifier,
    BlockIdentifier,
    ExprIdentifier,

    Number,
    String,

    Plus,
    PlusEquals,
    PlusPlus,
    Minus,
    MinusEquals,
    MinusMinus,
    Star,
    StarEquals,
    Slash,
    SlashEquals,
    Modulo,
    ModuloEquals,
    Power,
    PowerEquals,

    And,
    Is,
    Not,
    CouldBe,
    Class,
    Fn,
    Let,
    Def,
    Import,
    From,
    Export,
    As,
    If,
    For,
    Do,
    While,
    Then,
    Else,
    Finally,
    Log,
    Return,

    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,
    Comma,
    Period,
    Colon,
    Semicolon,
    GreaterThan,
    GreaterEqual,
    LessThan,
    LessEqual,
    Equals,
};

struct FilePos {
    std::size_t line_no{1}, column_no{1};
};

struct FilePosFactory {
    void move(char c);

    FilePos current{};
};

struct Token {
    Token(TokenType type, FilePos pos);
    Token(TokenType type, std::string text, FilePos pos);
    Token(double number, FilePos pos);

    TokenType type{TokenType::Illegal};
    struct {
        std::string lexeme;
        double number;
    } literal{};
    FilePos file_pos{};
};

struct TokenFactory {
    void feed(char c);

    Token compile(TokenType type);

    std::stringstream builder{};
    FilePosFactory file_pos_factory{};
};

#endif //DSCRIPT_TOKEN_H