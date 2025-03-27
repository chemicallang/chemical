// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 28/02/2024.
//

#ifdef COMPILER_BUILD
#include <llvm/TargetParser/Host.h>
#endif
#include "parser/Parser.h"
#include "utils/Environment.h"
#include "compiler/InvokeUtils.h"
#include "compiler/Codegen.h"
#include "compiler/SymbolResolver.h"
#include "utils/CmdUtils.h"
#include <filesystem>
#include "preprocess/RepresentationVisitor.h"
#include "utils/PathUtils.h"
#include <functional>
#include "preprocess/2c/2cASTVisitor.h"
#include "compiler/ASTProcessor.h"
#include "integration/libtcc/LibTccInteg.h"
#include "utils/Version.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "rang.hpp"
#ifdef _WIN32
#include <crtdbg.h>
#endif

#ifdef COMPILER_BUILD

int chemical_clang_main(int argc, char **argv);

int chemical_clang_main2(const std::vector<std::string> &command_args);

int llvm_ar_main2(const std::span<chem::string_view> &command_args);

#endif

void print_usage() {
    std::string usage = "chemical <input_file> -o <output_file>\n\n";
    usage += "<input_file> a chemical file path with .ch extension relative to current executable\n";
    usage += "<output_file> a .o (object) file or a .ll (llvm ir) file\n";
    std::cout << usage;
}

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
                 "configure           -[empty]      configures the compiler for this OS\n"
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
                 "--debug-ir          -[empty]      output llvm ir, even with errors, for debugging\n"
                 "--ignore-errors     -[empty]      ignore any errors that happen and compile any way\n"
                 "--arg-[arg]         -arg-[arg]    can be used to provide arguments to build.lab\n"
//                 "--verify            -o            do not compile, only verify source code\n"
                 "--jit               -jit          do just in time compilation using Tiny CC\n"
                 "--no-cbi            -[empty]      this ignores cbi annotations when translating\n"
                 "--no-caching        -[empty]      no caching will be done\n"
                 "--cpp-like          -[empty]      configure output of c translation to be like c++\n"
                 "--res <dir>         -res <dir>    change the location of resources directory\n"
                 "--benchmark         -bm           benchmark lexing / parsing / compilation process\n"
                 "--print-ig          -pr-ig        print import graph of the source file\n"
                 "--print-ast         -pr-ast       print representation of AST\n"
                 "--print-cst         -pr-cst       print CST for debugging\n"
                 "" << std::endl;

}

void config_job_success_msg(int job, const std::string& msg) {
    std::cout << rang::fg::green << '[' << job << ']' << ' ' << msg << rang::fg::reset << std::endl;
}

void config_job_error_msg(int job, const std::string& msg) {
    std::cout << rang::fg::red << '[' << job << ']' << ' ' << msg << rang::fg::reset << std::endl;
}

void config_job_info_msg(int job, const std::string& msg) {
    std::cout << rang::fg::gray << '[' << job << ']' << ' ' << msg << rang::fg::reset << std::endl;
}

struct Version {
    int major; int minor; int patch;
    /**
     * 1 when this version is greater
     * 0 when this version is smaller
     * -1 when both are equal
     */
    [[nodiscard]]
    int compare(const Version& other) const {
        if(major == other.major) {
            if(minor == other.minor) {
                if(patch == other.patch) {
                    return -1;
                } else {
                    return patch > other.patch;
                }
            } else {
                return minor > other.minor;
            }
        } else {
            return major > other.major;
        }
    }
    std::string formatted() {
        return std::to_string(major) + '.' + std::to_string(minor) + '.' + std::to_string(patch);
    }
};

Version parse_version(const std::string& version) {
    Version result = { -1, -1, -1 };  // Default for invalid input
    std::istringstream stream(version);
    char dot;
    if (!(stream >> result.major)) return result;
    if (!(stream >> dot) || dot != '.') return result;
    if (!(stream >> result.minor)) return result;
    if (!(stream >> dot) || dot != '.') return result;
    if (!(stream >> result.patch)) return result;
    return result;
}

int configure_exe(CmdOptions& options, int argc, char* argv[]) {

    // checking if has sudo or admin privileges
    int job = 1;
    bool stay = false;

#ifdef _WIN32
    if(isAdmin()) {
        config_job_success_msg(job++, "Successfully gained administrator privileges.");
        stay = true;
    } else {
        config_job_info_msg(job++, "Relaunching as administrator");
        if(relaunchAsAdmin()) {
            return 0;
        } else {
            config_job_error_msg(job++, "Couldn't relaunch as administrator");
            return 1;
        }
    }
#else
    if(isSudo()) {
        config_job_success_msg(job++, "Successfully gained administrator privileges.");
    } else {
        if(requestSudo()) {
            config_job_success_msg(job++, "Successfully gained sudo");
        } else {
            config_job_error_msg(job++, "Failure to gain sudo");
            return 1;
        }
    }
#endif

    auto parent_path = resolve_parent_path(argv[0]);
    if(set_environment_variable("CHEMICAL_HOME", parent_path, false)) {
        config_job_success_msg(job++, "Set 'CHEMICAL_HOME' environment variable to '" + parent_path + "'");
    } else {
        config_job_error_msg(job++, "Couldn't set 'CHEMICAL_HOME' environment variable to '" + parent_path + "'");
//        return 1;
    }

    auto existing_version = get_chemical_version_in_PATH();
    if(existing_version.empty()) {
        if (add_to_PATH(parent_path, false)) {
            config_job_success_msg(job++, "Added chemical to PATH environment variable");
        } else {
            config_job_error_msg(job++, "Couldn't add chemical to PATH environment variable" + parent_path + "'");
//            return 1;
        }
    } else {
        auto current = Version { PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH };
        auto existing = parse_version(existing_version);
        auto compare = current.compare(existing);
        if(compare == -1) {
            config_job_info_msg(job++, "Chemical already exists on PATH with same version v" + existing.formatted());
        } else if(compare == 0) {
            config_job_info_msg(job++, "Chemical version v" + existing.formatted() + " newer than current v "+ current.formatted() + " already exists in PATH");
        } else {
            config_job_error_msg(job++, "Chemical version v" + existing.formatted() + " older than current v" + current.formatted() + " is present in PATH environment variable, please remove it");
        }
//        return 1;
    }

    std::cout << rang::fg::green << "\nSuccessfully configured Chemical Compiler for your OS" << rang::fg::reset << std::endl;

    if(stay) {
        std::cin.get();
    }
    return 0;
}

const auto include_cmd_desc = "include a c header or a chemical file in compilation";
const auto build_dir_desc = "specify a build directory to output build files or create module directory";
const auto link_lib_cmd_desc = "link the given library when compiling";
const auto cc_cmd_desc = "invokes the cc tool";
const auto configure_cmd_desc = "configures the compiler for this OS";
const auto linker_cmd_desc = "invoke the linker to link libraries";
const auto ar_cmd_desc = "invoke the ar tool to archive libraries";
const auto dlltool_cmd_desc = "invoke the dlltool to archive libraries";
const auto ranlib_cmd_desc = "invoke the llvm ranlib tool";
const auto lib_cmd_desc = "invoke the llvm lib tool";
const auto mode_cmd_desc = "mode: debug, debug_quick, release_small, release_fast";
const auto version_cmd_desc = "get the version of the compiler";
const auto help_cmd_desc = "get help for command line options";
const auto benchmark_cmd_desc = "benchmark the compilation process";
const auto print_ast_desc = "print representation of the ast";
const auto print_cst_desc = "print representation of the cst";
const auto print_ig_desc = "print representation of the import graph";
const auto verbose_desc = "verbose enables complete logging";
const auto ignore_errors_desc = "ignore certain errors during compilation";
const auto lto_desc = "enable link time optimization";
const auto assertions_desc = "enable assertions for checking of generated code";
const auto no_pie_desc = "disable position independent code";
const auto target_desc = "the target for which code is being generated";
const auto jit_desc = "just in time compile the given input";
const auto output_desc = "the output at which file(s) will be generated";
const auto resources_desc = "the path to resources directory required";
const auto ignore_extension_desc = "compiler will ignore the extension of the file";
const auto ll_out_desc = "specify output path for .ll (llvm ir) file";
const auto bc_out_desc = "specify output path for .bc (bitcode) file";
const auto obj_out_desc = "specify output path for .o (object) file";
const auto asm_out_desc = "specify output path for .s (assembly) file";
const auto bin_out_desc = "specify output path for binary file";
const auto debug_ir_desc = "set debug mode for generated llvm ir";
const auto dash_c_desc = "generate objects without linking them into final executable";
const auto no_caching_desc = "no caching will be done for future invocations";
const auto cbi_m_desc = "compile a compiler binding interface that provides support for macros";
const auto fno_unwind_desc = "no unwind tables would be generated";
const auto mod_f_desc = "compile a file as a module, the argument must be in format <mod-name>:<file-path>";
const auto mod_d_desc = "compile a directory as a module, the argument must be in format <mod-name>:<dir-path>";
const auto out_dash_all_desc = "generate a corresponding file for every additional module specific via --mod";

inline std::vector<std::string_view>& get_includes(CmdOptions& options) {
    return options.data.find("include")->second.multi_value.values;
}

void take_include_options(LabModule& module, CmdOptions& options) {
    auto& includes = get_includes(options);
    for(auto& value : includes) {
        if(value.ends_with(".ch")) {
            module.paths.emplace_back(std::string(value));
        } else {
            module.headers.emplace_back(std::string(value));
        }
    }
}

void take_linked_libs(LabJob& job, CmdOptions& options) {
    const auto& libs = options.data.find("library")->second.multi_value.values;
    for(auto& lib : libs) {
        job.linkables.emplace_back(chem::string::make_view(lib));
    }
    const auto& libs2 = options.data.find("l")->second.multi_value.values;
    for(auto& lib : libs2) {
        job.linkables.emplace_back(chem::string::make_view(lib));
    }
}

LabModule* create_or_find_module(std::vector<std::unique_ptr<LabModule>>& modules, const chem::string_view& name, const chem::string_view& path, LabModuleType mod_type) {
    // we use the previously created module with same name, if it's a files module (to append the file to same module created before)
    if(mod_type == LabModuleType::Files) {
        for (auto& mod: modules) {
            if (mod->name.to_chem_view() == name) {
                return mod.get();
            }
        }
    }
    const auto mod = new LabModule(mod_type, chem::string(""), chem::string(name));
    mod->paths.emplace_back(path);
    modules.emplace_back(mod);
    return mod;
}

void include_mod_command_modules(
    std::vector<std::unique_ptr<LabModule>>& modules,
    const std::string_view& command_key,
    std::vector<std::string_view>& command_values,
    LabJob& job,
    LabModule* main_mod,
    LabModuleType mod_type
) {
    if(command_values.empty()) return;
    for(auto& lib : command_values) {
        auto found = lib.find(':');
        if(found != std::string::npos) {
            auto name = chem::string_view(lib.data(), found);
            auto path = chem::string_view(lib.data() + (found + 1));
            const auto mod = create_or_find_module(modules, name, path, mod_type);
            if(mod_type == LabModuleType::Directory) {
                job.path_aliases[name.str()] = path.str();
            }
            main_mod->add_dependency(mod);
        } else {
            std::cerr << rang::fg::red << "the argument to --" << command_key << " must be formatted as <name>:<path>" << rang::fg::reset;
        }
    }
}

void include_mod_d_modules(std::vector<std::unique_ptr<LabModule>>& modules, CmdOptions& options, LabJob& job, LabModule* main_mod) {
    auto& libs = options.data.find("mod-d")->second.multi_value.values;
    include_mod_command_modules(modules, "mod-d", libs, job, main_mod, LabModuleType::Directory);
}
void include_mod_f_modules(std::vector<std::unique_ptr<LabModule>>& modules, CmdOptions& options, LabJob& job, LabModule* main_mod) {
    auto& libs = options.data.find("mod-f")->second.multi_value.values;
    include_mod_command_modules(modules, "mod-f", libs, job, main_mod, LabModuleType::Files);
}

void set_options_for_main_job(CmdOptions& options, LabJob& job, LabModule& module, std::vector<std::unique_ptr<LabModule>>& dependencies) {

    // set the build directory for the job
    const auto build_dir = options.option_new("build-dir");
    if(build_dir.has_value()) {
        job.build_dir = chem::string::make_view(build_dir.value());
    }

    take_include_options(module, options);
    take_linked_libs(job, options);

    auto start = dependencies.size(); // where additional modules begin

    include_mod_d_modules(dependencies, options, job, &module);
    include_mod_f_modules(dependencies, options, job, &module);

    // setting output for ll, bc, obj and asm files for corresponding modules
    const auto has_ll = options.has_value("out-ll-all");
    const auto has_asm = options.has_value("out-asm-all");
    const auto size = dependencies.size();
    while(start < size) {
        const auto mod = dependencies[start].get();
        const auto mod_dir = resolve_rel_child_path_str(job.build_dir.to_std_string(), mod->name.to_std_string());
        if(has_ll) {
            mod->llvm_ir_path.append(resolve_rel_child_path_str(mod_dir, "llvm_ir.ll"));
        }
        if(has_asm) {
            mod->asm_path.append(resolve_rel_child_path_str(mod_dir, "mod_asm.s"));
        }
        start++;
    }

}

void build_cbi_modules(LabBuildCompiler& compiler, CmdOptions& options) {
    auto& libs = options.data.find("cbi-m")->second.multi_value.values;
    for(auto& lib : libs) {
        auto found = lib.find(':');
        if(found != std::string::npos) {
            auto name = chem::string_view(lib.data(), found);
            auto path = chem::string_view(lib.data() + (found + 1));

            // creating the job and module and setting options for it
            LabJob job(LabJobType::CBI, chem::string(name), chem::string(path), chem::string(compiler.options->build_dir));
            LabModule mod(LabModuleType::Directory, chem::string(""), chem::string(name));
            mod.paths.emplace_back(path);
            job.dependencies.emplace_back(&mod);
            std::vector<std::unique_ptr<LabModule>> dependencies;
            set_options_for_main_job(options, job, mod, dependencies);

            compiler.do_job_allocating(&job);
            auto cbiDataItr = compiler.binder.data.find(name.str());
            if(cbiDataItr != compiler.binder.data.end()) {
                auto& cbiData = cbiDataItr->second;
                if(cbiData.module == nullptr) {
                    std::cerr << rang::fg::red << "cbi with name '" << name << "' doesn't have any data present" << rang::fg::reset << std::endl;
                    continue;
                }
                const auto module = cbiData.module;
                auto sym = tcc_get_symbol(module, "initializeLexer");
                if(!sym) {
                    std::cerr << rang::fg::red << "cbi with name '" << name << "' doesn't contain function 'initializeLexer'" << rang::fg::reset << std::endl;
                    continue;
                }
                auto sym2 = tcc_get_symbol(module, "parseMacroValue");
                if(!sym2) {
                    std::cerr << rang::fg::red << "cbi with name '" << name << "' doesn't contain function 'parseMacroValue'" << rang::fg::reset << std::endl;
                    continue;
                }
                auto sym3 = tcc_get_symbol(module, "parseMacroNode");
                if(!sym3) {
                    std::cerr << rang::fg::red << "cbi with name '" << name << "' doesn't contain function 'parseMacroNode'" << rang::fg::reset << std::endl;
                    continue;
                }
                auto& cbi_name_ref = cbiDataItr->first;
                auto cbi_name = chem::string_view(cbi_name_ref.data(), cbi_name_ref.size());
                compiler.binder.initializeLexerFunctions[cbi_name] = (UserLexerInitializeFn) sym;
                compiler.binder.parseMacroValueFunctions[cbi_name] = (UserParserParseMacroValueFn) sym2;
                compiler.binder.parseMacroNodeFunctions[cbi_name] = (UserParserParseMacroNodeFn) sym3;
            }
        } else {
            std::cerr << rang::fg::red << "the argument to --cbi must be formatted as <name>:<directory_path>" << rang::fg::reset;
        }
    }
}

int main(int argc, char *argv[]) {

// enable this code if debugging heap allocations is required
//#ifdef _WIN32
//    // Enable debug heap allocations and automatic leak checking
//    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
//    tmpFlag |= _CRTDBG_ALLOC_MEM_DF;      // Use debug heap allocations.
//    tmpFlag |= _CRTDBG_CHECK_ALWAYS_DF;   // Perform a heap consistency check after every allocation/deallocation.
//    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;     // Dump memory leaks when the program exits.
//    _CrtSetDbgFlag(tmpFlag);
//#endif

#ifdef COMPILER_BUILD
    // invoke clang cc1, this is used by clang, because it invokes (current executable)
    if(argc >= 2 && strcmp(argv[1], "-cc1") == 0) {
        return chemical_clang_main(argc, argv);
    }
#endif

    // parsing the command
    CmdOptions options;
    CmdOption cmd_data[] = {
        CmdOption("include", CmdOptionType::MultiValued, include_cmd_desc),
        CmdOption("build-dir", CmdOptionType::SingleValue, build_dir_desc),
        CmdOption("library", "l", CmdOptionType::MultiValued, link_lib_cmd_desc),
        CmdOption("cc", CmdOptionType::SubCommand, cc_cmd_desc),
        CmdOption("configure", CmdOptionType::SubCommand, configure_cmd_desc),
        CmdOption("linker", CmdOptionType::SubCommand, linker_cmd_desc),
        CmdOption("ar", CmdOptionType::SubCommand, ar_cmd_desc),
        CmdOption("dlltool", CmdOptionType::SubCommand, dlltool_cmd_desc),
        CmdOption("ranlib", CmdOptionType::SubCommand, ranlib_cmd_desc),
        CmdOption("lib", CmdOptionType::SubCommand, lib_cmd_desc),
        CmdOption("mode", "m", CmdOptionType::SingleValue, mode_cmd_desc),
        CmdOption("version", CmdOptionType::NoValue, version_cmd_desc),
        CmdOption("help", CmdOptionType::NoValue, help_cmd_desc),
        CmdOption("benchmark", "bm", CmdOptionType::NoValue, benchmark_cmd_desc),
        CmdOption("print-ast", "pr-ast", CmdOptionType::NoValue, print_ast_desc),
        CmdOption("print-cst", "pr-cst", CmdOptionType::NoValue, print_cst_desc),
        CmdOption("print-ig", "pr-ig", CmdOptionType::NoValue, print_ig_desc),
        CmdOption("verbose", "v", CmdOptionType::NoValue, verbose_desc),
        CmdOption("ignore-errors", "ignore-errors", CmdOptionType::NoValue, ignore_errors_desc),
        CmdOption("lto", CmdOptionType::NoValue, lto_desc),
        CmdOption("assertions", CmdOptionType::NoValue, assertions_desc),
        CmdOption("no-pie", "no-pie", CmdOptionType::NoValue, no_pie_desc),
        CmdOption("target", "t", CmdOptionType::SingleValue, target_desc),
        CmdOption("jit", "jit", CmdOptionType::NoValue, jit_desc),
        CmdOption("output", "o", CmdOptionType::SingleValue, output_desc),
        CmdOption("resources", "res", CmdOptionType::SingleValue, resources_desc),
        CmdOption("ignore-extension", CmdOptionType::NoValue, ignore_extension_desc),
        CmdOption("no-caching", CmdOptionType::NoValue, no_caching_desc),
        CmdOption("out-ll", CmdOptionType::SingleValue, ll_out_desc),
        CmdOption("out-bc", CmdOptionType::SingleValue, bc_out_desc),
        CmdOption("out-obj", CmdOptionType::SingleValue, obj_out_desc),
        CmdOption("out-asm", CmdOptionType::SingleValue, asm_out_desc),
        CmdOption("out-bin", CmdOptionType::SingleValue, bin_out_desc),
        CmdOption("out-ll-all", CmdOptionType::NoValue, out_dash_all_desc),
        CmdOption("out-asm-all", CmdOptionType::NoValue, out_dash_all_desc),
        CmdOption("debug-ir", CmdOptionType::NoValue, debug_ir_desc),
        CmdOption("", "c", CmdOptionType::NoValue, dash_c_desc),
        CmdOption("cbi-m", "cbi-m", CmdOptionType::MultiValued, cbi_m_desc),
        CmdOption("", "fno-unwind-tables", CmdOptionType::NoValue, fno_unwind_desc),
        CmdOption("", "fno-asynchronous-unwind-tables", CmdOptionType::NoValue, fno_unwind_desc),
        CmdOption("mod-f", "", CmdOptionType::MultiValued, mod_f_desc),
        CmdOption("mod-d", "", CmdOptionType::MultiValued, mod_d_desc),
    };
    options.register_options(cmd_data, sizeof(cmd_data) / sizeof(CmdOption));
    options.parse_cmd_options(argc, argv, 1);
    auto& args = options.arguments;

    // check if configure is called
    auto& config_cmd_opt = options.cmd_opt("configure");
    if(config_cmd_opt.has_multi_value()) {
        return configure_exe(options, argc, argv);
    }

    if(options.has_value("version")) {
        std::cout << "Chemical v" << PROJECT_VERSION_MAJOR << "." << PROJECT_VERSION_MINOR << "." << PROJECT_VERSION_PATCH << std::endl;
        return 0;
    }

#ifdef COMPILER_BUILD
    auto llvm_tool = [](int argc, char** argv, CmdOptions& options, const std::string_view& option) -> int {
        auto& cmd_opt = options.cmd_opt(option);
        if(cmd_opt.has_multi_value() && !cmd_opt.multi_value.values.empty()) {
            std::vector<chem::string_view> subc;
            subc.emplace_back(option);
            cmd_opt.put_multi_value_vec(subc);
            return llvm_ar_main2(subc);
        } else {
            return -999;
        }
    };
    const auto dll_tool_ret = llvm_tool(argc, argv, options, "dlltool");
    if(dll_tool_ret != -999) return dll_tool_ret;
    const auto ran_lib_ret = llvm_tool(argc, argv, options, "ranlib");
    if(ran_lib_ret != -999) return ran_lib_ret;
    const auto lib_ret = llvm_tool(argc, argv, options, "lib");
    if(lib_ret != -999) return lib_ret;
    const auto ar_ret = llvm_tool(argc, argv, options, "ar");
    if(ar_ret != -999) return ar_ret;
#endif

#ifdef COMPILER_BUILD
    // use raw clang
    auto& cc_cmd_opt = options.cmd_opt("cc");
    if(!cc_cmd_opt.multi_value.values.empty()) {
        std::vector<std::string> subc;
        subc.emplace_back(argv[0]);
        cc_cmd_opt.put_multi_value_vec(subc);
//        std::cout << "rclg  : ";
//        for(const auto& sub : subc) {
//            std::cout << sub;
//        }
//        std::cout << std::endl;
        return chemical_clang_main2(subc);
    }
#endif

    auto verbose = options.has_value("verbose", "v");

    if(options.has_value("help")) {
        print_help();
        return 0;
    }

    if(args.empty() && get_includes(options).empty()) {
        std::cerr << rang::fg::red << "no input given\n\n" << rang::fg::reset;
        print_usage();
        return 1;
    }

    bool jit = options.has_value("jit", "jit");
    auto& output = options.option_new("output", "o");
    auto& res = options.option_new("resources", "res");
    auto& dash_c = options.option_new("", "c");

#ifdef COMPILER_BUILD

    auto get_resources_path = [&res, &argv]() -> std::string {
        auto resources_path = res.has_value() ? std::string(res.value()) : resources_path_rel_to_exe(std::string(argv[0]));
        if(resources_path.empty()) {
            std::cerr << rang::fg::yellow << "warning: " << rang::fg::reset;
            std::cerr << "couldn't locate resources path relative to compiler's executable" << std::endl;
        }
        return resources_path;
    };

#endif

    auto prepare_options = [&](LabBuildCompilerOptions* opts) -> void {
        opts->benchmark = options.has_value("benchmark", "bm");
        opts->print_representation = options.has_value("print-ast", "pr-ast");
        opts->print_cst = options.has_value("print-cst", "pr-cst");
        opts->print_ig = options.has_value("print-ig", "pr-ig");
        opts->verbose = verbose;
#ifdef COMPILER_BUILD
        opts->resources_path = get_resources_path();
#endif
        opts->ignore_errors = options.has_value("ignore-errors", "ignore-errors");
        if(options.has_value("no-caching")) {
            opts->is_caching_enabled = false;
        }
        if(options.has_value("debug-ir")) {
            opts->debug_ir = true;
        }
//        opts->isCBIEnabled = !options.option("no-cbi").has_value();
        if(options.has_value("lto")) {
            opts->def_lto_on = true;
        }
        if(options.has_value("assertions")) {
            opts->def_assertions_on = true;
        }
#ifdef COMPILER_BUILD
        if(options.has_value("no-pie", "no-pie")) {
            opts->no_pie = true;
        }
#endif
    };

#ifdef COMPILER_BUILD

    // get and print target
    std::string target;
    auto& user_target = options.option_new("target", "t");
    if(user_target.has_value()) {
        target = user_target.value();
    } else {
        target = llvm::sys::getDefaultTargetTriple();
    }
    if(verbose) {
        std::cout << "target: " << target << std::endl;
    }

    // determine if is 64bit
    bool is64Bit = Codegen::is_arch_64bit(target);

#else
    std::string target = "native";
#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
    bool is64Bit = true;
#else
    bool is64Bit = false;
#endif
#endif

    OutputMode mode = OutputMode::Debug;

    // configuring output mode from command line
    auto& mode_opt = options.option_new("mode", "m");
    if(mode_opt.has_value()) {
        if(mode_opt.value() == "debug") {
            // ignore
        } else if(mode_opt.value() == "debug_quick") {
            mode = OutputMode::DebugQuick;
            if(verbose) {
                std::cout << "mode: Debug Quick Enabled (debug_quick)" << std::endl;
            }
        } else if(mode_opt.value() == "debug_complete") {
            mode = OutputMode::DebugComplete;
            if(verbose) {
                std::cout << "mode: Debug Complete Enabled (debug_complete)" << std::endl;
            }
        } else if(mode_opt.value() == "release" || mode_opt.value() == "release_fast") {
            mode = OutputMode::ReleaseFast;
            if(verbose) {
                std::cout << "mode: Release Fast Enabled (release_fast)" << std::endl;
            }
        } else if(mode_opt.value() == "release_small") {
            if(verbose) {
                std::cout << "mode: Release Small Enabled (release_small)" << std::endl;
            }
            mode = OutputMode::ReleaseSmall;
        } else {
            std::cerr << rang::fg::yellow << "warning: " << rang::fg::reset;
            std::cout << "unknown mode '" << mode_opt.value() << '\'' << std::endl;
        }
    }
#ifdef DEBUG
    else {
        std::cout << "mode: Debug Quick Enabled (debug_quick)" << std::endl;
        mode = OutputMode::DebugQuick;
    }
#endif

    auto build_dir_opt = options.option_new("build-dir", "b");

    // build a .lab file
    const auto is_lab_file = args[0].ends_with(".lab");
    const auto is_mod_file = args[0].ends_with(".mod");
    if(is_lab_file || is_mod_file) {

        std::string build_dir = build_dir_opt.has_value() ? std::string(build_dir_opt.value()) : resolve_non_canon_parent_path(std::string(args[0]), "build");
        LabBuildCompilerOptions compiler_opts(argv[0], target, std::move(build_dir), is64Bit);
        CompilerBinder binder(argv[0]);
        LabBuildCompiler compiler(binder, &compiler_opts);
        compiler.set_cmd_options(&options);

        // Prepare compiler options
        prepare_options(&compiler_opts);

        // build cbi modules
        build_cbi_modules(compiler, options);

        // translate the build.lab to a c file (for debugging)
        if(output.has_value() && output.value().ends_with(".c")) {
            LabJob job(LabJobType::ToCTranslation, chem::string("[BuildLabTranslation]"), chem::string(output.value()), chem::string(compiler_opts.build_dir));
            LabModule module(LabModuleType::Files, chem::string(""), chem::string("[BuildLabFile]"));
            module.paths.emplace_back(std::string(args[0]));
            job.dependencies.emplace_back(&module);
            std::vector<std::unique_ptr<LabModule>> dependencies;
            set_options_for_main_job(options, job, module, dependencies);
            return compiler.do_job_allocating(&job);
        }

        LabBuildContext context(
                compiler,
                compiler.path_handler,
                compiler.mod_storage,
                compiler.binder,
                std::string(args[0])
        );
        compiler_opts.outMode = mode;

        // giving build args to lab build context
        if(output.has_value()) {
            context.build_args["output"] = output.value();
        }
        if(mode_opt.has_value()) {
            context.build_args["mode"] = mode_opt.value();
        }
        for(auto& opt : options.options) {
            if(opt.first.starts_with("arg-")) {
                context.build_args[opt.first.data() + 4] = opt.second;
            }
        }

        if(is_lab_file) {
            // building the lab file
            const auto result = compiler.build_lab_file(context, args[0]);
            return result;
        } else {
            // building the mod file
            chem::string outputPath;
            if(output.has_value()) {
                outputPath.append(output.value());
            } else {
#ifdef _WIN32
                outputPath.append("a.exe");
#else
                outputPath.append("a");
#endif
            }
            const auto result = compiler.build_mod_file(context, args[0], std::move(outputPath));
            return result;
        }

    }

    // compilation
    std::string build_dir = build_dir_opt.has_value() ? std::string(build_dir_opt.value()) : "./";
    LabBuildCompilerOptions compiler_opts(argv[0], target, std::move(build_dir), is64Bit);
    CompilerBinder binder(argv[0]);
    LabBuildCompiler compiler(binder, &compiler_opts);
    compiler.set_cmd_options(&options);

    // set default compiler options
    compiler_opts.is_caching_enabled = false;
    compiler_opts.outMode = mode;

    prepare_options(&compiler_opts);

    // build cbi modules
    build_cbi_modules(compiler, options);

    // if not empty, this file will be removed at the end
    std::string_view temporary_obj;

    LabModule module(LabModuleType::Files, chem::string(""), chem::string("main"));

    // setting extra files to emit, like ll, bc, obj, asm (absolute paths)
    auto& ll_out = options.option_new("out-ll");
    auto& bc_out = options.option_new("out-bc");
    auto& obj_out = options.option_new("out-obj");
    auto& asm_out = options.option_new("out-asm");
    auto& bin_out = options.option_new("out-bin");

    if(ll_out.has_value())
        module.llvm_ir_path = chem::string::make_view(ll_out.value());
    if(bc_out.has_value())
        module.bitcode_path = chem::string::make_view(bc_out.value());
    if(obj_out.has_value())
        module.object_path = chem::string::make_view(obj_out.value());
    if(asm_out.has_value())
        module.asm_path = chem::string::make_view(asm_out.value());

    // setting output flag according to extension
    if(output.has_value()) {
        if(!options.has_value("ignore-extension")) {
            auto& output_val = output.value();
            // determining output file based on extension
            if (output_val.ends_with(".o")) {
                module.object_path = chem::string::make_view(output.value());
            } else if (output_val.ends_with(".s")) {
                module.asm_path = chem::string::make_view(output.value());
            } else if (output_val.ends_with(".ll")) {
                module.llvm_ir_path = chem::string::make_view(output.value());
            } else if (output_val.ends_with(".bc")) {
                module.bitcode_path = chem::string::make_view(output.value());
            } else if(!bin_out.has_value()) {
                bin_out.emplace(output.value());
            }
        } else if(!bin_out.has_value()) {
            bin_out.emplace(output.value());
        }
    } else if(!bin_out.has_value()){
        // clang also outputs a.exe, so we will too
#ifdef _WIN32
        bin_out.emplace("a.exe");
#else
        bin_out.emplace("a");
#endif
    }

    // have object file output for the binary we are outputting
    if (module.object_path.empty() && bin_out.has_value()) {
        module.object_path.append(bin_out.value());
        module.object_path.append(std::string_view(".o"));
        if(!dash_c.has_value()) {
            temporary_obj = module.object_path.to_view();
        }
    }

    std::vector<std::unique_ptr<LabModule>> dependencies;
    for(auto& arg : args) {
        if(arg.ends_with(".c")) {
            auto dep = new LabModule(LabModuleType::CFile, chem::string(""), chem::string(""));
            dependencies.emplace_back(dep);
            dep->paths.emplace_back(std::string(arg));
            module.add_dependency(dep);
        } else if(arg.ends_with(".o")) {
            auto dep = new LabModule(LabModuleType::ObjFile, chem::string(""), chem::string(""));
            dependencies.emplace_back(dep);
            dep->paths.emplace_back(std::string(arg));
            module.add_dependency(dep);
        } else {
            module.paths.emplace_back(std::string(arg));
        }
    }

    LabJob job(LabJobType::Executable, chem::string("a"));
    set_options_for_main_job(options, job, module, dependencies);

    // checking if user requires ll, asm output at default location for the module
    const auto has_ll = options.has_value("out-ll-all");
    const auto has_asm = options.has_value("out-asm-all");
    if(has_ll || has_asm) {
        const auto mod_dir = resolve_rel_child_path_str(job.build_dir.to_std_string(), module.name.to_std_string());
        if (has_ll) {
            module.llvm_ir_path.append(resolve_rel_child_path_str(mod_dir, "llvm_ir.ll"));
        }
        if (has_asm) {
            module.asm_path.append(resolve_rel_child_path_str(mod_dir, "mod_asm.s"));
        }
    }

    if(dash_c.has_value() || !bin_out.has_value()) {
        job.type = LabJobType::ProcessingOnly;
    } else if(output.has_value()) {
        if(output.value().ends_with(".c")) {
            job.type = LabJobType::ToCTranslation;
        } else if(output.value().ends_with(".ch")) {
            job.type = LabJobType::ToChemicalTranslation;
        }
    }
    job.dependencies.emplace_back(&module);
    if(output.has_value()) {
        job.abs_path.append(std::string(output.value()));
    }
    auto return_int = compiler.do_job_allocating(&job);

    // delete object file which was linked
    if(!temporary_obj.empty()) {
        try {
            std::filesystem::remove(temporary_obj);
        } catch (const std::filesystem::filesystem_error &ex) {
            std::cerr << rang::fg::red << "error: couldn't delete temporary object file " << temporary_obj << " because " << ex.what() << rang::fg::reset << std::endl;
            return_int = 1;
        }
    }

    return return_int;

}