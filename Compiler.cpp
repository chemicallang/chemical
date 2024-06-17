// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#ifdef COMPILER_BUILD
#include <llvm/TargetParser/Host.h>
#endif
#include "lexer/Lexi.h"
#include "utils/Utils.h"
#include "ast/base/GlobalInterpretScope.h"
#include "compiler/Codegen.h"
#include "compiler/SymbolResolver.h"
#include "utils/CmdUtils.h"
#include "cst/base/CSTConverter.h"
#include <filesystem>
#include "utils/StrUtils.h"
#include "preprocess/ImportGraphMaker.h"
#include "compiler/IGCompiler.h"
#include "preprocess/ToCTranslator.h"
#include "preprocess/RepresentationVisitor.h"
#include "preprocess/SourceVerifier.h"
#include "utils/PathUtils.h"
#include <functional>
#include "preprocess/2cASTVisitor.h"
#include "compiler/ASTProcessor.h"
#include "integration/libtcc/LibTccInteg.h"
#include "utils/Version.h"

#ifdef COMPILER_BUILD

int chemical_clang_main(int argc, char **argv);

int chemical_clang_main2(const std::vector<std::string> &command_args);

std::vector<std::unique_ptr<ASTNode>> TranslateC(const char* exe_path, const char *abs_path, const char *resources_path);

#endif

void print_help() {
    std::cout << "[Chemical] "
                 #ifdef COMPILER_BUILD
                 "Compiler v"
                 #endif
                 #ifdef TCC_BUILD
                 "Compiler based on Tiny CC v"
                 #endif
                 << PROJECT_VERSION_MAJOR << "." << PROJECT_VERSION_MINOR << "." << PROJECT_VERSION_PATCH << "\n"
                 << std::endl;
    std::cout << "Compiling a single file : \nchemical.exe <input_filename> -o <output_filename>\n\n"
                 "<input_filename> extensions supported .c , .h , .ch\n"
                 "<output_filename> extensions supported .exe, .o, .c, .ch\n"
                 "use input extension .c and output .ch, when translating C code to Chemical\n"
                 "use input extension .ch and output .c, when translating Chemical to C code\n\n"
                 "Invoke Clang : \nchemical.exe cc <clang parameters>\n\n"
                 "--mode              -m            debug or release mode : debug, release, debug_quick, release_aggressive\n"
                 "--verify            -o            do not compile, only verify source code\n"
                 "--jit               -jit          do just in time compilation using Tiny CC\n"
                 "--res <dir>         -res <dir>    change the location of resources directory\n"
                 "--benchmark         -bm           benchmark lexing / parsing / compilation process\n"
                 "--print-ig          -pr-ig        print import graph of the source file\n"
                 "--print-ast         -pr-ast       print representation of AST\n"
                 "--print-cst         -pr-cst       print CST for debugging\n"
                 "" << std::endl;

}

int main(int argc, char *argv[]) {

#ifdef COMPILER_BUILD
    // invoke clang cc1, this is used by clang, because it invokes (current executable)
    if(argc >= 2 && strcmp(argv[1], "-cc1") == 0) {
        return chemical_clang_main(argc, argv);
    }
#endif

    // parsing the command
    CmdOptions options;
    auto args = options.parse_cmd_options(argc, argv, 1, {"cc", "clang", "linker", "jit"});

#ifdef COMPILER_BUILD
    // use raw clang
    auto rawclang = options.option("cc", "cc");
    if(rawclang.has_value()) {
        auto subc = options.collect_subcommand(argc, argv, "cc");
        subc.insert(subc.begin(), argv[0]);
//        std::cout << "rclg  : ";
//        for(const auto& sub : subc) {
//            std::cout << sub;
//        }
//        std::cout << std::endl;
        return chemical_clang_main2(subc);
    }
#endif

    auto verbose = options.option("verbose", "v").has_value();
    if(verbose) {
        std::cout << "[Command] ";
        options.print();
        std::cout << std::endl;
    }

    if(options.option("help", "help").has_value()) {
        print_help();
        return 0;
    }

    if(args.empty()) {
        std::cerr << cmd_error("no input given\n\n");
        print_usage();
        return 1;
    }

    auto srcFilePath = args[0];

    auto only_verify = options.option("verify", "verify").has_value();
    auto benchmark = options.option("benchmark", "bm").has_value();
    auto print_ig = options.option("print-ig", "pr-ig").has_value();
    auto print_representation = options.option("print-ast", "pr-ast").has_value();
    auto print_ir = options.option("print-ir", "pr-ir").has_value();
    auto print_cst = options.option("print-cst", "pr-cst").has_value();
    bool jit = options.option("jit", "jit").has_value();
    auto output = options.option("output", "o");
    auto res = options.option("res", "res");

    auto get_resources_path = [&res, &argv]() -> std::string{
        auto resources_path = res.has_value() ? res.value() : resources_path_rel_to_exe(std::string(argv[0]));
        if(resources_path.empty()) {
            std::cerr << "[Compiler] Couldn't locate resources path relative to compiler's executable" << std::endl;
        } else if(!res.has_value()) {
            res.emplace(resources_path);
        }
        return resources_path;
    };

    auto prepare_options = [&](ASTProcessorOptions* options) -> void {
        options->benchmark = benchmark;
        options->print_representation = print_representation;
        options->print_cst = print_cst;
        options->print_ig = print_ig;
        options->verbose = verbose;
        options->resources_path = get_resources_path();
    };

#ifdef COMPILER_BUILD

    // get and print target
    auto target = options.option("target", "t");
    if (!target.has_value()) {
        target.emplace(llvm::sys::getDefaultTargetTriple());
    }
    if(verbose) {
        std::cout << "[Target] " << target.value() << std::endl;
    }

    // determine if is 64bit
    bool is64Bit = Codegen::is_arch_64bit(target.value());

#else
    bool is64Bit = false;
#endif

    // do not compile
    if(only_verify) {
        SourceVerifierOptions verify_opts(argv[0]);
        prepare_options(&verify_opts);
        if(verify(srcFilePath, &verify_opts)) {
            return 0;
        } else {
            return 1;
        }
    }

    // translate chemical to C
    auto tcc = options.option("tcc", "tcc").has_value();
#ifdef COMPILER_BUILD
    if(jit || (output.has_value() && (tcc || output.value().ends_with(".c") || output.value().ends_with(".h")))) {
#endif
        ToCTranslatorOptions translator_opts(argv[0], is64Bit);
        prepare_options(&translator_opts);
        auto translator_preparer = [&options](ToCAstVisitor* visitor, ASTProcessor* processor) -> void {
            visitor->inline_struct_members_fn_types = !options.option("take-out-struct-member-fn-types").has_value();
            visitor->cpp_like = options.option("cpp-like").has_value();
            processor->lexer->isCBIEnabled = !options.option("no-cbi").has_value();
        };
        if(output.has_value() && (output.value().ends_with(".c") || output.value().ends_with(".h")) && !jit) {
            bool good = translate(srcFilePath, output.value(), &translator_opts, translator_preparer);
            return good ? 0 : 1;
        } else {
            std::string cProgramStr;
            if(srcFilePath.ends_with(".c") || srcFilePath.ends_with(".h")) {
                std::ifstream input;
                input.open(srcFilePath);
                if(!input.is_open()) {
                    std::cerr << "[Compiler] couldn't open the file at location " << srcFilePath << std::endl;
                    return 1;
                }
                std::stringstream buffer;
                buffer << input.rdbuf();
                cProgramStr = std::move(buffer.str());
                input.close();
            } else {
                cProgramStr = translate(srcFilePath, &translator_opts, translator_preparer);
                if(cProgramStr.empty()) {
                    return 1;
                }
            }
            return compile_c_string(argv[0], cProgramStr.data(), output.has_value() ? output.value() : "", jit, benchmark);
        }

#ifdef COMPILER_BUILD
    }
#endif
#ifdef COMPILER_BUILD

    // translate C to chemical
    if((srcFilePath.ends_with(".c") || srcFilePath.ends_with(".h")) && output.has_value() && output.value().ends_with(".ch")) {
        auto nodes = TranslateC(argv[0], srcFilePath.c_str(), get_resources_path().c_str());
        // write translated to the given file
        std::ofstream out;
        out.open(output.value());
        if(!out.is_open()) {
            std::cout << "[TranslateC] Couldn't open the file at path " << output.value() << std::endl;
            return 1;
        }
        RepresentationVisitor visitor(out);
        visitor.translate(nodes);
        out.close();
        // verify if required
        if(only_verify) {
            SourceVerifierOptions verify_opts(argv[0]);
            prepare_options(&verify_opts);
            if(!verify(output.value(), &verify_opts)) {
                return 1;
            }
        }
        return 0;
    }

    // compilation
    Codegen gen({}, srcFilePath, target.value(), argv[0], is64Bit);
    IGCompilerOptions compiler_opts(argv[0], target.value(), is64Bit);
    prepare_options(&compiler_opts);
    if(!compile(&gen, srcFilePath, &compiler_opts)) {
        return 1;
    }

    // check if it requires printing
    if (print_ir) {
        // print to console
        gen.print_to_console();
    }

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
        gen.save_to_ll_file(output.value());
        options.print_unhandled();
        return 0;
    } else if(endsWith(output.value(), ".bc")) {
        gen.save_to_bc_file(output.value());
        options.print_unhandled();
        return 0;
    }

    // creating object file for compilation
    std::string object_file_path = output.value() + ".o";
    gen.save_to_object_file(object_file_path);

    int return_int = 0;

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

#endif

}