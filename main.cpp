#include <iostream>
#include <string>
#include <vector>

#include "lexing/lexer.h"
#include "parsing/parser.h"
#include "resolving/resolver.h"
#include "runtime/vm.h"

int main(const int argc, char **argv) {
    const std::string script_path = argc > 1 ? argv[1] : "example/ex1.dsr";

    FileLexer lexer(script_path);

    const std::vector<StmtPtr> statements = parse(lexer);
    if (lexer.had_error) {
        std::cerr << "Lexing failed for " << script_path << "\n";
        return 1;
    }

    Resolver resolver;
    resolver.resolve(statements);
    if (resolver.had_error()) {
        for (const auto &error : resolver.get_errors()) {
            std::cerr << error << '\n';
        }
        return 1;
    }

    Vm vm;
    vm.set_source(script_path);
    vm.set_locals(resolver.get_locals());
    vm.interpret(statements);
    (void) vm.invoke_main_if_present();

    return 0;
}