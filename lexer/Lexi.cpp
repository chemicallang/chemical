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

Lexer benchLexFile(unsigned int file_id, InputSource& source, LocationManager& manager) {
    SourceProvider reader(&source);
    Lexer lexer(file_id, reader, manager);
    BenchmarkResults results{};
    benchLex(&lexer, results);
    std::cout << "[Lex]" << " Completed " << results.representation() << std::endl;
    return lexer;
}

Lexer benchLexFile(unsigned int file_id, const char* path, LocationManager& manager) {
    FileInputSource input_source(path);
    return benchLexFile(file_id, input_source, manager);
}

Lexer lexFile(unsigned int file_id, InputSource& input_source, LocationManager& manager) {
    SourceProvider reader(&input_source);
    Lexer lexer(file_id, reader, manager);
    lexer.lex();
    return lexer;
}

Lexer lexFile(unsigned int file_id, const char* path, LocationManager& manager) {
    FileInputSource input_source(path);
    return lexFile(file_id, input_source, manager);
}