#include "lexing/lexer.h"

int main() {
    std::vector<Token> tokens = StringLexer{"log 'hi!';"}.tokenize();
}