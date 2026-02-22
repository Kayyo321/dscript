cmake_minimum_required(VERSION 4.1)
project(dscript)

set(CMAKE_CXX_STANDARD 20)

add_executable(dscript main.cpp
        lexing/token.h
        lexing/token.cpp
        lexing/lexer.h
        lexing/lexer.cpp
        runtime/obj.cpp
        runtime/objs.cpp
        runtime/value.cpp
        runtime/value.h
        runtime/obj.h
        runtime/objs.h
        parsing/stmt.h
        parsing/a_visitor.h
        parsing/expr.h
        parsing/parser.cpp
        parsing/parser.h
        runtime/vm.h
        runtime/env.h
        runtime/instance.h
        runtime/env.cpp
        runtime/vm.cpp
        runtime/stdlibs/core.cpp
        runtime/stdlibs/io.cpp
        runtime/stdlibs/registry.cpp
        runtime/klass.h
        resolving/resolver.h
        resolving/resolver.cpp)
