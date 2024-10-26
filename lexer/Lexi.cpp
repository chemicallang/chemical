// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 21/02/2024.
//

#include "Lexi.h"
#include "lexer/Lexer.h"
#include "stream/SourceProvider.h"
#include "utils/Benchmark.h"
#include "stream/FileInputSource.h"
#include <iostream>

void benchLex(Lexer *lexer, BenchmarkResults &results) {
    results.benchmark_begin();
    lexer->lex();
    results.benchmark_end();
}

void benchLexFile(Lexer* lexer, const char* path, BenchmarkResults& results) {
    FileInputSource input_source(path);
    lexer->provider.switch_source(&input_source);
    benchLex(lexer, results);
}

void benchLexFile(Lexer* lexer, const char* path) {
    BenchmarkResults results{};
    benchLexFile(lexer, path, results);
}

void lexFile(Lexer* lexer, const char* path) {
    FileInputSource input_source(path);
    lexer->provider.switch_source(&input_source);
    lexer->lex();
}

Lexer benchLexFile(const std::string_view& path, InputSource& source) {
    SourceProvider reader(&source);
    Lexer lexer(std::string(path), reader);
    BenchmarkResults results{};
    benchLex(&lexer, results);
    std::cout << "[Lex]" << " Completed " << results.representation() << std::endl;
    return lexer;
}

Lexer benchLexFile(const std::string_view& path) {
    FileInputSource input_source(path.data());
    return benchLexFile(path, input_source);
}

Lexer lexFile(const std::string_view& path, InputSource& input_source) {
    SourceProvider reader(&input_source);
    Lexer lexer(std::string(path), reader);
    lexer.lex();
    return lexer;
}

Lexer lexFile(const std::string_view& path) {
    FileInputSource input_source(path.data());
    return lexFile(path, input_source);
}