//
// Created by sullivanb on 2/20/26.
//

#ifndef DSCRIPT_LEXER_H
#define DSCRIPT_LEXER_H

#include "token.h"

#include <fstream>
#include <vector>
#include <map>

class Lexer {
public:
    virtual ~Lexer() = default;

    virtual char peek(int offset) = 0;
    virtual char next(bool feed) = 0;

    std::vector<Token> tokenize();

    bool had_error {false};

private:
    Token next_token();
    Token string();
    Token number();
    Token identifier(char start = ' ');
    Token symbol();

    void skip_whitespace();
    bool is_at_end();

    Token eat_then_compile(TokenType type);
    Token eat_then_compile(TokenType type, std::size_t bytes);

    const std::map<std::string, TokenType> keywords = {
        {"and", TokenType::And},
        {"is", TokenType::Is},
        {"not", TokenType::Not},
        {"couldbe", TokenType::CouldBe},
        {"class", TokenType::Class},
        {"fn", TokenType::Fn},
        {"let", TokenType::Let},
        {"def", TokenType::Def},
        {"import", TokenType::Import},
        {"from", TokenType::From},
        {"export", TokenType::Export},
        {"as", TokenType::As},
        {"if", TokenType::If},
        {"for", TokenType::For},
        {"do", TokenType::Do},
        {"while", TokenType::While},
        {"then", TokenType::Then},
        {"else", TokenType::Else},
        {"finally", TokenType::Finally},
        {"log", TokenType::Log},
        {"return", TokenType::Return},
    };

protected:
    TokenFactory token_factory{};
};

class StringLexer: public Lexer {
public:
    explicit StringLexer(std::string sv);

    char peek(int offset) override;
    char next(bool feed) override;

private:
    std::string text{};
    std::string::const_iterator iter{nullptr};
};

class FileLexer: public Lexer {
public:
    explicit FileLexer(const std::string &path);

    char peek(int offset) override;
    char next(bool feed) override;

private:
    std::ifstream file{};
};

#endif //DSCRIPT_LEXER_H