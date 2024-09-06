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

void benchLexFile(Lexer* lexer, const std::string &path, BenchmarkResults& results) {
    FileInputSource input_source(path);
    lexer->provider.switch_source(&input_source);
    benchLex(lexer, results);
}

void benchLexFile(Lexer* lexer, const std::string &path) {
    BenchmarkResults results{};
    benchLexFile(lexer, path, results);
}

void lexFile(Lexer* lexer, const std::string &path) {
    FileInputSource input_source(path);
    lexer->provider.switch_source(&input_source);
    lexer->lex();
}

Lexer benchLexFile(InputSource& source) {
    SourceProvider reader(&source);
    Lexer lexer(reader);
    BenchmarkResults results{};
    benchLex(&lexer, results);
    std::cout << "[Lex]" << " Completed " << results.representation() << std::endl;
    return lexer;
}

Lexer benchLexFile(const std::string &path) {
    FileInputSource input_source(path);
    return benchLexFile(input_source);
}

Lexer lexFile(InputSource& input_source) {
    SourceProvider reader(&input_source);
    Lexer lexer(reader);
    lexer.lex();
    return lexer;
}

Lexer lexFile(const std::string &path) {
    FileInputSource input_source(path);
    return lexFile(input_source);
}