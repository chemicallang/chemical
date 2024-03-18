// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include <llvm/TargetParser/Host.h>
#include "lexer/Lexi.h"
#include "parser/Persi.h"
#include <cstdio>
#include "utils/Utils.h"
#include "ast/utils/ExpressionEvaluator.h"
#include "ast/utils/ValueType.h"
#include "ast/base/GlobalInterpretScope.h"
#include "compiler/Codegen.h"
#include "utils/CmdUtils.h"
#include <filesystem>

#define DEBUG true

int chemical_clang_main(int argc, char **argv);

bool endsWith(const std::string &fullString, const std::string &ending) {
    if (fullString.length() >= ending.length()) {
        return (fullString.compare(fullString.length() - ending.length(), ending.length(), ending) == 0);
    } else {
        return false;
    }
}

int main(int argc, char *argv[]) {

    if (argc == 0) {
        std::cerr << "No inputs given\n\n";
        print_usage();
        return 1;
    }

    // print command in debug mode
    if(DEBUG) {
        std::cout << "Command : ";
        int i = 0;
        while (i < argc) {
            std::cout << argv[i] << ' ';
            i++;
        }
        std::cout << std::endl;
    }

    // invoke clang cc1, this is used by clang, because it invokes (current executable)
    if(strcmp(argv[1], "-cc1") == 0) {
        if(DEBUG) std::cout << "Invoking clang cc1: ";
        return chemical_clang_main(argc, argv);
    }

    // parsing the command
    CmdOptions options;
    options.parse_cmd_options(argc, argv, 1);

    if(DEBUG) {
        std::cout << "Formed Command : ";
        options.print();
        std::cout << std::endl;
    }

    if (options.count_args() == 0) {
        std::cerr << "No inputs given to the compiler\n\n";
        print_usage();
        return 1;
    }

    // Lex, parse & type check
    auto lexer = benchLexFile(argv[1]);
//    printTokens(lexer.tokens);
    for (const auto &err: lexer.errors) {
        std::cerr << err.representation(argv[1]) << std::endl;
    }
    auto parser = benchParse(std::move(lexer.tokens));
    for (const auto &err: parser.errors) {
        std::cerr << err.representation(argv[1]) << std::endl;
    }
    TypeChecker checker;
    checker.type_check(parser.nodes);
    for (const auto &err: checker.errors) {
        std::cerr << err << std::endl;
    }
    Scope scope(std::move(parser.nodes));
//    std::cout << "[Representation]\n" << scope.representation() << std::endl;
    if (!lexer.errors.empty() || !parser.errors.empty() || !checker.errors.empty()) return 1;

    // actual compilation
    Codegen gen(std::move(scope.nodes), argv[1]);
    gen.compile();

    // check if it requires printing
    auto print = options.option("print-ir", "pir");
    if (print.has_value()) {
        // print to console
        gen.print_to_console();
    }

    // get and print target
    auto target = options.option("target", "t");
    if (!target.has_value()) {
        target.emplace(llvm::sys::getDefaultTargetTriple());
    }
    std::cout << "Target: " << target.value() << std::endl;

    int return_int = 0;
    auto output = options.option("output", "o");
    if (!output.has_value()) {
        output.emplace("compiled");
    }

    // writing object / ll file when user wants only that !
    if(endsWith(output.value(), ".o")) {
        gen.save_to_object_file(output.value(), target.value());
        return 0;
    } else if(endsWith(output.value(), ".ll")) {
        gen.save_to_file(output.value(), target.value());
        return 0;
    }

    // creating object file for compilation
    std::string object_file_path = output.value() + ".o";
    gen.save_to_object_file(object_file_path, target.value());
    if (!gen.errors.empty()) {
        gen.print_errors();
        return 1;
    }

    // check no need to invoke clang
    auto invoke_clang = options.option("use-clang", "use-clang");
    if (invoke_clang.has_value()) {
        std::vector<std::string> clang_flags{
                argv[0],
                object_file_path,
                "-o",
                output.value(),
                "--target",
                target.value(),
                "-v"
        };
        return_int = gen.invoke_clang(clang_flags);
    } else {

        // creating lld command
        std::vector<std::string> linker{object_file_path, "/OUT:"+output.value()};

        // link with standard libc (unless user opts out)
        auto option = options.option("no-libc", "no-libc");
        if(!option.has_value()) {
#if defined(_WIN32)
            linker.emplace_back("-defaultlib:libcmt");
#elif defined(__APPLE__)
            // TODO test linking with libc on apple
            linker.emplace_back("-lc");
#elif defined(__linux__)
            // TODO test linking with libc on linux
            linker.emplace_back("-lc");
#endif
        }

        // add user's linker flags
        auto user_flags = options.collect_multi("linker");
        for(const auto& flag : user_flags) {
            linker.emplace_back(flag);
        }

        // invoke lld to create executable
        return_int = gen.invoke_lld(linker);

        // delete object file which was linked
        // Attempt to delete the file using std::filesystem
        // TODO this doesn't work
        try {
            std::filesystem::remove(object_file_path);
        } catch (const std::filesystem::filesystem_error& ex) {
            std::cerr << "couldn't delete object file " << object_file_path << " because " << ex.what() << std::endl;
            return 1;
        }
    }

    return return_int;

}