//
// Created by sullivanb on 2/20/26.
//

#ifndef DSCRIPT_PARSER_H
#define DSCRIPT_PARSER_H

#include "stmt.h"
#include "../lexing/lexer.h"

std::vector<StmtPtr> parse(Lexer &lexer);

#endif //DSCRIPT_PARSER_H