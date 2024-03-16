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
    auto generate = options.option("gen", "g");
    if(generate.has_value()) {
        if (gen.errors.empty()) { // if there's no compilation errors
            if (endsWith(generate.value(), ".ll")) {
                // save the generation to a file
                gen.save_to_file(generate.value());
            } else if (endsWith(generate.value(), ".o")) {
                gen.save_to_object_file(generate.value());
            } else {
                std::cerr << "Unknown output file path given, the output file must have .ll or .o extension" << std::endl;
            }
        }
    }
    auto output = options.option("output", "o");
    if(output.has_value()) {
        std::string object_file_path;
        bool delete_object_default = true;
        if(generate.has_value() && endsWith(generate.value(), ".o")) {
            object_file_path = generate.value();
            delete_object_default = false;
        } else {
            object_file_path = output.value() + ".o";
            gen.save_to_object_file(object_file_path);
        }
        std::vector<std::string> link_objs;
        link_objs.push_back(object_file_path);
        std::vector<std::string> linker_flags{"-l:stdio.so"};
        gen.link_object_files_as_executable(link_objs, output.value(), linker_flags);
    }
    auto print = options.option("print-ir", "pir");
    if (print.has_value()) {
        // print to console
        gen.print_to_console();
    }
    // prints the errors occurred during saving as well
    for (const auto &error: gen.errors) {
        std::cout << error << std::endl;
    }
    return 0;
}