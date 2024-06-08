// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include <llvm/TargetParser/Host.h>
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

int chemical_clang_main(int argc, char **argv);

int chemical_clang_main2(const std::vector<std::string> &command_args);

std::vector<std::unique_ptr<ASTNode>> TranslateC(const char* exe_path, const char *abs_path, const char *resources_path);

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

    auto verbose = options.option("verbose", "v").has_value();
    if(verbose) {
        std::cout << "[Command] ";
        options.print();
        std::cout << std::endl;
    }

    if(args.empty()) {
        std::cerr << cmd_error("no input given\n\n");
        print_usage();
        return 1;
    }

    auto srcFilePath = args[0];

    auto help = options.option("help", "help").has_value();
    auto only_verify = options.option("verify", "verify").has_value();
    auto benchmark = options.option("benchmark", "bm").has_value();
    auto print_ig = options.option("print-ig", "pr-ig").has_value();
    auto print_representation = options.option("print-ast", "pr-ast").has_value();
    auto print_ir = options.option("print-ir", "pr-ir").has_value();
    auto print_cst = options.option("print-cst", "pr-cst").has_value();
    auto res = options.option("res", "res");
    auto resources_path = res.has_value() ? res.value() : "resources";

    auto prepare_options = [&](ASTProcessorOptions* options) -> void {
        options->benchmark = benchmark;
        options->print_representation = print_representation;
        options->print_cst = print_cst;
        options->print_ig = print_ig;
        options->verbose = verbose;
        options->resources_path = resources_path;
    };

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

    // translate chemical to C
    auto translateToC = options.option("t2c", "t2c");
    if(translateToC.has_value()) {
        ToCTranslatorOptions translator_opts(argv[0], translateToC.value(), is64Bit);
        prepare_options(&translator_opts);
        bool good = translate(srcFilePath, &translator_opts);
        return good ? 0 : 1;
    }

    // translate C to chemical
    auto translateC = options.option("tc", "tc");
    if(translateC.has_value()) {
        auto nodes = TranslateC(argv[0], srcFilePath.c_str(), resources_path.c_str());
        // write translated to the given file
        std::ofstream out;
        out.open(translateC.value());
        if(!out.is_open()) {
            std::cout << "[TranslateC] Couldn't open the file at path " << translateC.value() << std::endl;
            return 1;
        }
        RepresentationVisitor visitor(out);
        visitor.translate(nodes);
        out.close();
        // verify if required
        if(only_verify) {
            SourceVerifierOptions verify_opts(argv[0]);
            prepare_options(&verify_opts);
            if(!verify(translateC.value(), &verify_opts)) {
                return 1;
            }
        }
        return 0;
    }

    // do not compile
    if(only_verify) {
        SourceVerifierOptions verify_opts(argv[0]);
        prepare_options(&verify_opts);
        if(!verify(srcFilePath, &verify_opts)) {
            return 1;
        }
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

#ifdef FEAT_JUST_IN_TIME

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

#endif

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
    }
#ifdef FEAT_ASSEMBLY_GEN
    else if(endsWith(output.value(), ".s")) {
        gen.save_to_assembly_file(output.value());
        options.print_unhandled();
        return 0;
    }
#endif
#ifdef FEAT_LLVM_IR_GEN
    else if(endsWith(output.value(), ".ll")) {
        gen.save_to_file(output.value());
        options.print_unhandled();
        return 0;
    }
#endif
#ifdef FEAT_BITCODE_GEN
    else if(endsWith(output.value(), ".bc")) {
        gen.save_as_bc_file(output.value());
        options.print_unhandled();
        return 0;
    }
#endif

    // creating object file for compilation
    std::string object_file_path = output.value() + ".o";
    gen.save_to_object_file(object_file_path);

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