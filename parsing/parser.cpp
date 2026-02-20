//
// Created by sullivanb on 2/20/26.
//

#include "parser.h"

class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : tokens{std::move(tokens)} {}

    std::vector<StmtPtr> parse() {

    }

    StmtPtr parse_stmt() {
        
    }

private:
    std::vector<Token> tokens{};
};

std::vector<StmtPtr> parse(Lexer &lexer) {
    return Parser{lexer.tokenize()}.parse();
}
