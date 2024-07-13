// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#ifdef COMPILER_BUILD
#include <llvm/TargetParser/Host.h>
#endif
#include "lexer/Lexi.h"
#include "utils/Utils.h"
#include "compiler/InvokeUtils.h"
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
#include "compiler/lab/LabBuildCompiler.h"

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
                 "<input_filename> extensions supported .c , .ch\n"
                 "<output_filename> extensions supported .exe, .o, .c, .ch\n"
                 "use input extension .c and output .ch, when translating C code to Chemical\n"
                 "use input extension .ch and output .c, when translating Chemical to C code\n\n"
                 "Invoke Clang : \nchemical.exe cc <clang parameters>\n\n"
                 "--mode              -m            debug or release mode : debug, debug_quick, release_small, release_fast\n"
                 "--output            -o            specify a output file, output determined by it's extension\n"
                 "--out-ll  <path>    -[empty]      specify a path to output a .ll file containing llvm ir\n"
                 "--out-asm <path>    -[empty]      specify a path to output a .s file containing assembly\n"
                 "--out-bc  <path>    -[empty]      specify a path to output a .bc file containing llvm bitecode\n"
                 "--out-obj <path>    -[empty]      specify a path to output a .obj file\n"
                 "--out-bin <path>    -[empty]      specify a path to output a binary file\n"
                 "--ignore-extension  -[empty]      ignore the extension --output or -o option\n"
                 "--lto               -[empty]      force link time optimization\n"
                 "--assertions        -[empty]      enable assertions on generated code\n"
                 "--debug-ll          -[empty]      output llvm ir, even with errors, for debugging\n"
                 "--arg-[arg]         -arg-[arg]    can be used to provide arguments to build.lab\n"
                 "--verify            -o            do not compile, only verify source code\n"
                 "--jit               -jit          do just in time compilation using Tiny CC\n"
                 "--no-cbi            -[empty]      this ignores cbi annotations when translating\n"
                 "--cpp-like          -[empty]      configure output of c translation to be like c++\n"
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

    auto prepare_options = [&](ASTProcessorOptions* opts) -> void {
        opts->benchmark = benchmark;
        opts->print_representation = print_representation;
        opts->print_cst = print_cst;
        opts->print_ig = print_ig;
        opts->verbose = verbose;
        opts->resources_path = get_resources_path();
        opts->isCBIEnabled = !options.option("no-cbi").has_value();
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
    std::optional<std::string> target = "native";
    bool is64Bit = false;
#endif

    // do not compile
    if(only_verify) {
        SourceVerifierOptions verify_opts(argv[0], target.value(), is64Bit);
        prepare_options(&verify_opts);
        if(verify(srcFilePath, &verify_opts)) {
            return 0;
        } else {
            return 1;
        }
    }

    OutputMode mode = OutputMode::Debug;

    // configuring output mode from command line
    auto mode_opt = options.option("mode", "m");
    if(mode_opt.has_value()) {
        if(mode_opt.value() == "debug") {
            // ignore
        } else if(mode_opt.value() == "debug_quick") {
            mode = OutputMode::DebugQuick;
            if(verbose) {
                std::cout << "[Compiler] Debug Quick Enabled" << std::endl;
            }
        } else if(mode_opt.value() == "release" || mode_opt.value() == "release_fast") {
            mode = OutputMode::ReleaseFast;
            if(verbose) {
                std::cout << "[Compiler] Release Fast Enabled" << std::endl;
            }
        } else if(mode_opt.value() == "release_small") {
            if(verbose) {
                std::cout << "[Compiler] Release Small Enabled" << std::endl;
            }
            mode = OutputMode::ReleaseSmall;
        }
    }
#ifdef DEBUG
    else {
        mode = OutputMode::DebugQuick;
    }
#endif

    // translate chemical to C
    auto tcc = options.option("tcc", "tcc").has_value();
#ifdef COMPILER_BUILD
    if(jit || (output.has_value() && (tcc || output.value().ends_with(".c")))) {
#endif
        if(!srcFilePath.ends_with(".lab") || (output.has_value() && output.value().ends_with(".c"))) {
            ToCTranslatorOptions translator_opts(argv[0], target.value(), is64Bit);
            prepare_options(&translator_opts);
            auto translator_preparer = [&options](ToCAstVisitor *visitor, ASTProcessor *processor) -> void {
                visitor->inline_struct_members_fn_types = !options.option(
                        "take-out-struct-member-fn-types").has_value();
                visitor->cpp_like = options.option("cpp-like").has_value();
                if (options.option("no-cbi").has_value()) {
                    processor->options->isCBIEnabled = false;
                }
            };
            if (output.has_value() && output.value().ends_with(".c") && !jit) {
                bool good = translate(srcFilePath, output.value(), &translator_opts, translator_preparer);
                return good ? 0 : 1;
            } else {
                std::string cProgramStr;
                if (srcFilePath.ends_with(".c")) {
                    auto read = read_file_to_string(srcFilePath.data());
                    if(read.has_value()) {
                        cProgramStr = std::move(read.value());
                    } else {
                        std::cerr << "[Compiler] couldn't open the file at location " << srcFilePath << std::endl;
                        return 1;
                    }
                } else {
                    cProgramStr = translate(srcFilePath, &translator_opts, translator_preparer);
                    if (cProgramStr.empty()) {
                        return 1;
                    }
                }
                return compile_c_string(argv[0], cProgramStr.data(), output.has_value() ? output.value() : "", jit,
                                        benchmark, is_debug(mode));
            }
        }

#ifdef COMPILER_BUILD
    }
#endif

    // build a .lab file
    if(srcFilePath.ends_with(".lab")) {
        LabBuildContext context(srcFilePath);
        LabBuildCompilerOptions compiler_opts(argv[0], target.value(), is64Bit);
        LabBuildCompiler compiler(&compiler_opts);
        prepare_options(&compiler_opts);
        compiler_opts.def_mode = mode;
        if(options.option("lto").has_value()) {
            compiler_opts.def_lto_on = true;
        }
        if(options.option("assertions").has_value()) {
            compiler_opts.def_assertions_on = true;
        }
        // giving build args to lab build context
        for(auto& opt : options.options) {
            if(opt.first.starts_with("arg-")) {
                context.build_args[opt.first.substr(4)] = opt.second;
            }
        }
        return compiler.build_lab_file(context, srcFilePath);
    }

#ifdef TCC_BUILD
    std::cerr << "Unknown Input / Output given to TCC based chemical compiler" << std::endl;
    return 1;
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
            SourceVerifierOptions verify_opts(argv[0], target.value(), is64Bit);
            prepare_options(&verify_opts);
            if(!verify(output.value(), &verify_opts)) {
                return 1;
            }
        }
        return 0;
    }

    // compilation
    Codegen gen({}, target.value(), argv[0], is64Bit, "");
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

    CodegenEmitterOptions emitter_options;
    configure_emitter_opts(mode, &emitter_options);

    auto ll_out = options.option("out-ll");
    auto bc_out = options.option("out-bc");
    auto obj_out = options.option("out-obj");
    auto asm_out = options.option("out-asm");
    auto bin_out = options.option("out-bin");

    // should the obj file be deleted after linking
    bool temporary_obj = false;

    if(output.has_value()) {
        if(!options.option("ignore-extension").has_value()) {
            // determining output file based on extension
            if (endsWith(output.value(), ".o")) {
                obj_out.emplace(output.value());
            } else if (endsWith(output.value(), ".s")) {
                asm_out.emplace(output.value());
            } else if (endsWith(output.value(), ".ll")) {
                ll_out.emplace(output.value());
            } else if (endsWith(output.value(), ".bc")) {
                bc_out.emplace(output.value());
            } else if(!bin_out.has_value()) {
                bin_out.emplace(output.value());
            }
        } else if(!bin_out.has_value()) {
            bin_out.emplace(output.value());
        }
    } else if(!bin_out.has_value()){
        bin_out.emplace("compiled");
    }

    // have object file output for the binary we are outputting
    if (!obj_out.has_value() && bin_out.has_value()) {
        obj_out.emplace(bin_out.value() + ".o");
        temporary_obj = true;
    }

    // files to emit
    if(ll_out.has_value())
        emitter_options.ir_path = ll_out.value().data();
    if(bc_out.has_value())
        emitter_options.bitcode_path = bc_out.value().data();
    if(obj_out.has_value())
        emitter_options.obj_path = obj_out.value().data();
    if(asm_out.has_value())
        emitter_options.asm_path = asm_out.value().data();

    // extra options to force lto or assertions
    if(options.option("lto").has_value()) {
        emitter_options.lto = true;
    }
    if(options.option("assertions").has_value()) {
        emitter_options.assertions_on = true;
    }

    if(ll_out.has_value() && options.option("debug-ll").has_value()) {
        gen.save_to_ll_file_for_debugging(ll_out.value());
        return 0;
    }

    int return_int = 0;

    // creating object file for compilation
    auto save_result = gen.save_with_options(&emitter_options);
    if(!save_result) {
        return_int = 1;
    }

    if(!bin_out.has_value()) {
        return return_int;
    }

    auto useLinker = options.option("linker", "linker");
    if(useLinker.has_value()) {

        // creating lld command
        std::vector<std::string> linker{obj_out.value()};

        // set output
#if defined(_WIN32)
        linker.emplace_back("/OUT:"+bin_out.value());
#elif defined(__APPLE__)
        linker.emplace_back("-o");
        linker.emplace_back("./"+bin_out.value());
#elif defined(__linux__)
        linker.emplace_back("-o");
        linker.emplace_back("./"+bin_out.value());
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
        clang_flags.emplace_back(obj_out.value());
        clang_flags.emplace_back("-o");
        clang_flags.emplace_back(bin_out.value());
        return_int = gen.invoke_clang(clang_flags);
    }

    if(temporary_obj) {
        // delete object file which was linked
        // Attempt to delete the file using std::filesystem
        try {
            std::filesystem::remove(obj_out.value());
        } catch (const std::filesystem::filesystem_error &ex) {
            std::cerr << ANSI_COLOR_RED << "couldn't delete object file " << obj_out.value() << " because " << ex.what()
                      << ANSI_COLOR_RESET << std::endl;
            return_int = 1;
        }
    }

    options.print_unhandled();
    return return_int;

#endif

}