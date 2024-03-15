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
#include "compiler/Codegen.h"
#include "utils/CmdUtils.h"

bool endsWith(const std::string &fullString, const std::string &ending) {
    if (fullString.length() >= ending.length()) {
        return (fullString.compare(fullString.length() - ending.length(), ending.length(), ending) == 0);
    } else {
        return false;
    }
}

int main(int argc, char *argv[]) {
    if (argc == 0) {
        std::cerr << "A file path argument is required so the file can be parsed\n\n";
        print_usage();
        return 1;
    }
    auto options = parse_cmd_options(argc, argv, 1);
    options.print();
    if (options.arguments.empty()) {
        std::cerr << "A source file argument must be given";
        print_usage();
        return 1;
    }
    auto output = options.option("output", "o");
    if (!output.has_value()) {
        std::cerr << "A output file path argument must be given\n\n";
        print_usage();
        return 1;
    }
    auto lexer = benchLexFile(argv[1]);
//    printTokens(lexer.tokens);
    for (const auto &err: lexer.errors) {
        std::cerr << err.representation() << std::endl;
    }
    auto parser = benchParse(std::move(lexer.tokens));
    for (const auto &err: parser.errors) {
        std::cerr << err;
    }
    TypeChecker checker;
    checker.type_check(parser.nodes);
    for (const auto &err: checker.errors) {
        std::cerr << err << std::endl;
    }
    Scope scope(std::move(parser.nodes));
//    std::cout << "[Representation]\n" << scope.representation() << std::endl;
    if (!lexer.errors.empty() || !parser.errors.empty() || !checker.errors.empty()) return 1;
    Codegen gen(std::move(scope.nodes), argv[1]);
    // compile
    gen.compile();
    if(gen.errors.empty()) { // if there's no compilation errors
        if (endsWith(output.value(), ".ll")) {
            // save the generation to a file
            gen.save_to_file(output.value());
        } else if (endsWith(output.value(), ".o")) {
            gen.save_to_object_file(output.value());
        }
    }
    // prints the errors occurred during saving as well
    for (const auto &error: gen.errors) {
        std::cout << error << std::endl;
    }
    auto print = options.option("print", "p");
    if (print.has_value()) {
        // print to console
        gen.print_to_console();
    }
    return 0;
}