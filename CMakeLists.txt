cmake_minimum_required(VERSION 4.1)
project(dscript)

set(CMAKE_CXX_STANDARD 20)

add_executable(dscript main.cpp
        app/cli.h
        app/cli.cpp
        app/artifact_loader.h
        app/artifact_loader.cpp
        app/json_builder.h
        app/json_builder.cpp
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
        runtime/stdlibs/math.cpp
        runtime/stdlibs/random.cpp
        runtime/stdlibs/registry.cpp
        runtime/stdlibs/math.h
        runtime/klass.h
        resolving/resolver.h
        resolving/resolver.cpp)

file(GLOB DSCRIPT_STDLIB_SOURCES "${CMAKE_SOURCE_DIR}/stdlibs/dsr/*.dsr")
set(DSCRIPT_STDLIB_ARTIFACTS)

foreach(STDLIB_SOURCE ${DSCRIPT_STDLIB_SOURCES})
        get_filename_component(STDLIB_NAME ${STDLIB_SOURCE} NAME_WE)
        set(STDLIB_ARTIFACT "${CMAKE_SOURCE_DIR}/stdlibs/dsar/${STDLIB_NAME}.dsar")

        add_custom_command(
                OUTPUT ${STDLIB_ARTIFACT}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/stdlibs/dsar"
                COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR}
                                $<TARGET_FILE:dscript>
                                --build
                                stdlibs/dsr/${STDLIB_NAME}.dsr
                                stdlibs/dsar/${STDLIB_NAME}.dsar
                COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${STDLIB_ARTIFACT}
                DEPENDS dscript ${STDLIB_SOURCE}
                COMMENT "Precompiling DScript stdlib: ${STDLIB_NAME}"
                VERBATIM
        )

        list(APPEND DSCRIPT_STDLIB_ARTIFACTS ${STDLIB_ARTIFACT})
endforeach()

if(DSCRIPT_STDLIB_ARTIFACTS)
        add_custom_target(precompile_dscript_stdlibs ALL DEPENDS ${DSCRIPT_STDLIB_ARTIFACTS})
endif()
