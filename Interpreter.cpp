// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "lexer/Lexi.h"
#include "utils/Utils.h"
#include "ast/utils/ExpressionEvaluator.h"
#include "ast/base/GlobalInterpretScope.h"
#include "cst/base/CSTConverter.h"
#include "compiler/SymbolResolver.h"
#include <chrono>

int main(int argc, char *argv[]) {
    if (argc == 0) {
        std::cout << "A file path argument is required so the file can be parsed";
        return 0;
    }
    auto srcFilePath = argv[1];
    auto lexer = benchLexFile(srcFilePath);
//    printTokens(lexer.tokens);
    for(const auto& err : lexer.errors) {
        std::cerr << err.representation(argv[1], "Lexer") << std::endl;
    }
    CSTConverter converter(true, "interpreter");
    converter.convert(lexer.tokens);
    for(const auto& err : converter.diagnostics) {
        std::cerr << err.representation(argv[1], "Parser") << std::endl;
    }
    Scope scope(std::move(converter.nodes));
//    std::cout << "[Representation]\n" << scope.representation() << "\n";
    GlobalInterpretScope interpretScope(nullptr, &scope, nullptr, argv[1]);
    define_all(interpretScope);


    // Print started
    // std::cout << "[Interpreter] Started" << std::endl;

    // Save start time
    auto start = std::chrono::steady_clock::now();

    // Actual interpretation
    {
        SymbolResolver linker(argv[0], srcFilePath, true);
        for(const auto& func : interpretScope.global_fns) {
            linker.declare(func.first, func.second.get());
        }
        scope.link_asynchronously(linker);
        if(!linker.errors.empty()){
            for(const auto& err : linker.errors) {
                std::cerr << "[Linker] " << err.message << std::endl;
            }
            return 1;
        }
    }
    scope.interpret(interpretScope);

    // Save end time
    auto end = std::chrono::steady_clock::now();

    // Calculating duration in different units
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

    // Printing stats
    std::cout << std::endl << "[Interpreter] Completed " << ' ';
    std::cout << "[Nanoseconds:" << nanos << "]";
    std::cout << "[Microseconds:" << micros << "]";
    std::cout << "[Milliseconds:" << millis << "]" << std::endl;

    for(const auto& err : interpretScope.errors) {
        std::cerr << "[Interpreter] " << err << '\n';
    }

    return 0;
}