// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include <llvm/TargetParser/Host.h>
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
        std::cerr << err.representation(argv[1]) << std::endl;
    }
    auto parser = benchParse(std::move(lexer.tokens));
    for (const auto &err: parser.errors) {
        std::cerr << err << std::endl;
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
    // check if requires printing
    auto print = options.option("print-ir", "pir");
    if (print.has_value()) {
        // print to console
        gen.print_to_console();
    }
    auto target = options.option("target", "t");
    if (!target.has_value()) {
        target.emplace(llvm::sys::getDefaultTargetTriple());
    }
    std::cout << "Target: " << target.value() << std::endl;
    auto generate = options.option("gen", "g");
    if (generate.has_value()) {
        if (gen.errors.empty()) { // if there's no compilation errors
            if (endsWith(generate.value(), ".ll")) {
                // save the generation to a file
                gen.save_to_file(generate.value());
            } else if (endsWith(generate.value(), ".o")) {
                gen.save_to_object_file(generate.value(), target.value());
            } else {
                std::cerr << "unknown generate file path given, the output file must have .ll or .o extension"
                          << std::endl;
            }
            if (!gen.errors.empty()) {
                gen.print_errors();
                return 1;
            }
        }
    }
    int return_int = 0;
    auto output = options.option("output", "o");
    if (!output.has_value()) {
        output.emplace("compiled");
    }
    auto no_compile = options.option("no-compile", "no-compile");
    if(no_compile.has_value()) {
        return 0;
    }
    std::string object_file_path;
    bool delete_object_default = true;
    auto useLL = options.option("use-ll", "use-ll");
    if (generate.has_value() &&
        (endsWith(generate.value(), ".o") || (endsWith(generate.value(), ".ll") && useLL.has_value()))) {
        object_file_path = generate.value();
        delete_object_default = false;
    } else {
        object_file_path = output.value() + ".o";
        gen.save_to_object_file(object_file_path, target.value());
    }
    std::vector<std::string> link_objs;
    link_objs.push_back(object_file_path);
    auto invoke_linker = options.option("use-lld", "lld");
    if (invoke_linker.has_value()) {
        std::vector<std::string> linker{object_file_path, "-v", "-lc"};
        return_int = gen.invoke_lld(linker);
    } else {
        std::vector<std::string> clang_flags;
        if (endsWith(object_file_path, ".ll")) {
            clang_flags.emplace_back("-x");
            clang_flags.emplace_back("ir");
            clang_flags.push_back(object_file_path);
        } else {
            clang_flags.push_back(object_file_path);
        }
        clang_flags.emplace_back("-v");
        clang_flags.emplace_back("-o");
        clang_flags.emplace_back(
                "/mnt/d/Programming/Cpp/zig-bootstrap/chemical/sample/compiled-actual-file-from-compiler");
        return_int = gen.invoke_clang(clang_flags);
    }
    if (!gen.errors.empty()) {
        gen.print_errors();
        return 1;
    }
    return return_int;
}