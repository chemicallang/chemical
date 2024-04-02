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
#include "ast/base/GlobalInterpretScope.h"
#include "compiler/Codegen.h"
#include "utils/CmdUtils.h"
#include <filesystem>

int chemical_clang_main(int argc, char **argv);

int chemical_clang_main2(const std::vector<std::string> &command_args);

bool endsWith(const std::string &fullString, const std::string &ending) {
    if (fullString.length() >= ending.length()) {
        return (fullString.compare(fullString.length() - ending.length(), ending.length(), ending) == 0);
    } else {
        return false;
    }
}

int main(int argc, char *argv[]) {

    // invoke clang cc1, this is used by clang, because it invokes (current executable)
    if(argc >= 2 && strcmp(argv[1], "-cc1") == 0) {
        return chemical_clang_main(argc, argv);
    }

    // parsing the command
    CmdOptions options;
    auto args = options.parse_cmd_options(argc, argv, 1, {"rclg", "clang", "linker", "jit"});

    // use raw clang
    auto rawclang = options.option("rclg", "rclg");
    if(rawclang.has_value()) {
        auto subc = options.collect_subcommand(argc, argv, "rclg");
        subc.insert(subc.begin(), argv[0]);
//        std::cout << "rclg  : ";
//        for(const auto& sub : subc) {
//            std::cout << sub;
//        }
//        std::cout << std::endl;
        return chemical_clang_main2(subc);
    }

    auto verbose = options.option("verbose", "v");
    if(verbose.has_value()) {
        std::cout << "parsed command : ";
        options.print();
        std::cout << std::endl;
    }

    if(args.empty()) {
        std::cerr << cmd_error("no input given\n\n");
        print_usage();
        return 1;
    }

    // Lex, parse & type check
    auto benchmark = options.option("benchmark", "bm");
    Lexer lexer = benchmark.has_value() ? benchLexFile(args[0]) : lexFile(args[0]);
    if(verbose.has_value()) {
        printTokens(lexer.tokens);
    }
    for (const auto &err: lexer.errors) {
        std::cerr << err.representation(argv[1], "Lexer") << std::endl;
    }
    auto parser = benchmark.has_value() ? benchParse(std::move(lexer.tokens)) : parse(std::move(lexer.tokens));
    for (const auto &err: parser.errors) {
        std::cerr << err.representation(argv[1], "Parser") << std::endl;
    }
    TypeChecker checker;
    checker.type_check(parser.nodes);
    for (const auto &err: checker.errors) {
        std::cerr << err << std::endl;
    }
    Scope scope(std::move(parser.nodes));
    if(verbose.has_value()) {
        std::cout << "[Representation]\n" << scope.representation() << std::endl;
    }
    if (lexer.has_errors || !parser.errors.empty() || !checker.errors.empty()) return 1;

    // TODO typechecker should run after the linker runs

    // linking the nodes
    {
        ASTLinker linker;
        scope.declare_top_level(linker);
        if(!linker.errors.empty()) {
            for(const auto& err : linker.errors) {
                std::cerr << "[Linker] " << err << std::endl;
            }
            // TODO preventing linker to stop if errors occur, because we need std.io import to work !
//            return 1;
        }
    }

    // get and print target
    auto target = options.option("target", "t");
    if (!target.has_value()) {
        target.emplace(llvm::sys::getDefaultTargetTriple());
    }
    if(verbose.has_value()) {
        std::cout << "Target: " << target.value() << std::endl;
    }

    // actual compilation
    Codegen gen(std::move(scope.nodes), argv[1], target.value(), argv[0]);
    gen.compile();

    // check if it requires printing
    auto print = options.option("print-ir", "pir");
    if (print.has_value()) {
        // print to console
        gen.print_to_console();
    }

    auto jit = options.option("jit", "jit");
    if(jit.has_value()) {
        auto jit_commands = options.collect_subcommand(argc, argv, "jit");
        std::vector<const char*> jit_args;
        args.reserve(jit_commands.size());
        for(const auto& cmd : jit_commands) {
            jit_args.push_back(cmd.c_str());
        }
        gen.just_in_time_compile(jit_args);
        gen.print_errors();
        return 0;
    }

    int return_int = 0;
    auto output = options.option("output", "o");
    if (!output.has_value()) {
        output.emplace("compiled");
    }

    // writing object / ll file when user wants only that !
    if(endsWith(output.value(), ".o")) {
        gen.save_to_object_file(output.value());
        options.print_unhandled();
        return 0;
    } else if(endsWith(output.value(), ".s")) {
        gen.save_to_assembly_file(output.value());
        options.print_unhandled();
        return 0;
    } else if(endsWith(output.value(), ".ll")) {
        gen.save_to_file(output.value());
        options.print_unhandled();
        return 0;
    } else if(endsWith(output.value(), ".bc")) {
        gen.save_as_bc_file(output.value());
        options.print_unhandled();
        return 0;
    }

    // creating object file for compilation
    std::string object_file_path = output.value() + ".o";
    gen.save_to_object_file(object_file_path);
    if (!gen.errors.empty()) {
        gen.print_errors();
        return 1;
    }

    auto useLinker = options.option("linker", "linker");
    if(useLinker.has_value()) {

        // creating lld command
        std::vector<std::string> linker{object_file_path};

        // set output
#if defined(_WIN32)
        linker.emplace_back("/OUT:"+output.value());
#elif defined(__APPLE__)
        linker.emplace_back("-o");
        linker.emplace_back("./"+output.value());
#elif defined(__linux__)
        linker.emplace_back("-o");
        linker.emplace_back("./"+output.value());
#endif

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
        auto user_libs = options.collect_subcommand(argc, argv, "linker");
        // TODO test this
        for(const auto& flag : user_libs) {
            linker.emplace_back(flag);
        }

        // invoke lld to create executable
        return_int = gen.invoke_lld(linker);

    } else {
        // use clang by default
        std::vector<std::string> clang_flags{argv[0]};
        options.option("clang", "clang"); // consume clang cmd
        auto consumed = options.collect_subcommand(argc, argv, "clang");
        for(const auto& cland_fl : consumed) {
            clang_flags.emplace_back(cland_fl);
        }
        clang_flags.emplace_back(object_file_path);
        clang_flags.emplace_back("-o");
        clang_flags.emplace_back(output.value());
        return_int = gen.invoke_clang(clang_flags);
    }

    // delete object file which was linked
    // Attempt to delete the file using std::filesystem
    try {
        std::filesystem::remove(object_file_path);
    } catch (const std::filesystem::filesystem_error& ex) {
        std::cerr << ANSI_COLOR_RED << "couldn't delete object file " << object_file_path << " because " << ex.what() << ANSI_COLOR_RESET << std::endl;
        return_int = 1;
    }

    options.print_unhandled();

    return return_int;

}