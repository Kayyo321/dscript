cmake_minimum_required(VERSION 4.1)
project(dscript)

set(CMAKE_CXX_STANDARD 20)

add_executable(dscript main.cpp
        lexing/token.h
        lexing/token.cpp
        lexing/lexer.h
        lexing/lexer.cpp
        runtime/value.cpp
        runtime/value.h
        runtime/obj.h
        runtime/objs.h)
