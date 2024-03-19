// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "lexer/Lexi.h"
#include "parser/Persi.h"
#include "utils/Utils.h"
#include "ast/utils/ExpressionEvaluator.h"
#include "ast/utils/ValueType.h"
#include "ast/base/GlobalInterpretScope.h"

void benchInterpret(Scope& scope, GlobalInterpretScope& interpretScope) {

    // Print started
    // std::cout << "[Interpreter] Started" << std::endl;

    // Save start time
    auto start = std::chrono::steady_clock::now();

    // Actual interpretation
    ExpressionEvaluator::prepareFunctions();
    scope.interpret((InterpretScope &) interpretScope);

    // Save end time
    auto end = std::chrono::steady_clock::now();

    // Calculating duration in different units
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

    // Printing stats
    std::cout << "[Interpreter] Completed " << ' ';
    std::cout << "[Nanoseconds:" << nanos << "]";
    std::cout << "[Microseconds:" << micros << "]";
    std::cout << "[Milliseconds:" << millis << "]" << '\n';

}

int main(int argc, char *argv[]) {
    if (argc == 0) {
        std::cout << "A file path argument is required so the file can be parsed";
        return 0;
    }
    auto lexer = benchLexFile(argv[1]);
//    printTokens(lexer.tokens);
    for(const auto& err : lexer.errors) {
        std::cerr << err.representation(argv[1], "Lexer") << std::endl;
    }
    auto parser = benchParse(std::move(lexer.tokens));
    for(const auto& err : parser.errors) {
        std::cerr << err.representation(argv[1], "Parser") << std::endl;
    }
    Scope scope(std::move(parser.nodes));
//    std::cout << "[Representation]\n" << scope.representation() << "\n";
    GlobalInterpretScope interpretScope(nullptr, &scope, nullptr, argv[1]);
    benchInterpret(scope, interpretScope);
    std::cout << std::endl;
    for(const auto& err : interpretScope.errors) {
        std::cerr << "[Interpreter] " << err << '\n';
    }
    return 0;
}