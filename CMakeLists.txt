cmake_minimum_required(VERSION 3.26)
project(Compiler)

set(CMAKE_CXX_STANDARD 17)

add_executable(Compiler main.cpp
        SourceProvider.cpp
        SourceProvider.h
        lexer/Lexer.cpp
        lexer/Lexer.h
        lexer/model/KeywordToken.h
        lexer/model/LexToken.h
        lexer/model/IntToken.h
)
