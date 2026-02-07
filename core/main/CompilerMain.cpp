// Copyright (c) Chemical Language Foundation 2025.

#include "CompilerMain.h"
#include "parser/utils/parse_num.h"
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

int chemical_clang_main2(std::vector<chem::string>& command_args);

int llvm_ar_main2(const std::span<chem::string>& command_args);

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
                 "--plugin-mode       -pm           debug or release mode : debug, debug_quick, release_small, release_fast\n"
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

    auto chemical_exe_path = getExecutablePath();
    auto parent_path = resolve_parent_path(chemical_exe_path);
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

inline std::span<std::string_view> get_includes(CmdOptions& options) {
    return options.data.find("include")->second.get_multi_opt_values();
}

void take_include_options(LabModule& module, CmdOptions& options) {
    auto includes = get_includes(options);
    for(auto& value : includes) {
        if(value.ends_with(".ch")) {
            module.paths.emplace_back(std::string(value));
        } else {
            module.headers.emplace_back(std::string(value));
        }
    }
}

void take_linked_libs(LabJob& job, CmdOptions& options) {
    const auto& libs = options.data.find("library")->second.get_multi_opt_values();
    for(auto& lib : libs) {
        job.objects.emplace_back(chem::string::make_view(lib));
    }
    const auto& libs2 = options.data.find("l")->second.get_multi_opt_values();
    for(auto& lib : libs2) {
        job.objects.emplace_back(chem::string::make_view(lib));
    }
}

LabModule* find_module(std::vector<std::unique_ptr<LabModule>>& modules, const chem::string_view& scope_name, const chem::string_view& name) {
    for (auto& mod: modules) {
        if (mod->name == name && mod->scope_name == scope_name) {
            return mod.get();
        }
    }
    return nullptr;
}

LabModule* create_module(std::vector<std::unique_ptr<LabModule>>& modules, const chem::string_view& scope_name, const chem::string_view& name, const chem::string_view& path, LabModuleType mod_type) {
    const auto mod = new LabModule(mod_type, chem::string(scope_name), chem::string(name));
    mod->paths.emplace_back(path);
    modules.emplace_back(mod);
    return mod;
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

    // setting output for ll, bc, obj and asm files for corresponding modules
    const auto has_ll = options.has_value("out-ll-all");
    const auto has_asm = options.has_value("out-asm-all");
    const auto size = dependencies.size();
    while(start < size) {
        const auto mod = dependencies[start].get();
        const auto mod_dir = resolve_rel_child_path_str(job.build_dir.to_view(), mod->name.to_view());
        if(has_ll) {
            mod->llvm_ir_path.append(resolve_rel_child_path_str(mod_dir, "llvm_ir.ll"));
        }
        if(has_asm) {
            mod->asm_path.append(resolve_rel_child_path_str(mod_dir, "mod_asm.s"));
        }
        start++;
    }

}

OutputMode get_output_mode(std::optional<std::string_view>& mode_opt, bool verbose) {
    // configuring output mode from command line
    if(mode_opt.has_value()) {
        if(mode_opt.value() == "debug") {
            // ignore
        } else if(mode_opt.value() == "debug_quick") {
            if(verbose) {
                std::cout << "mode: Debug Quick Enabled (debug_quick)" << std::endl;
            }
            return OutputMode::DebugQuick;
        } else if(mode_opt.value() == "debug_complete") {
            if(verbose) {
                std::cout << "mode: Debug Complete Enabled (debug_complete)" << std::endl;
            }
            return OutputMode::DebugComplete;
        } else if(mode_opt.value() == "release" || mode_opt.value() == "release_fast") {
            if(verbose) {
                std::cout << "mode: Release Fast Enabled (release_fast)" << std::endl;
            }
            return OutputMode::ReleaseFast;
        } else if(mode_opt.value() == "release_small") {
            if(verbose) {
                std::cout << "mode: Release Small Enabled (release_small)" << std::endl;
            }
            return OutputMode::ReleaseSmall;
        } else {
            std::cerr << rang::fg::yellow << "warning: " << rang::fg::reset;
            std::cout << "unknown mode '" << mode_opt.value() << '\'' << std::endl;
        }
    }
#ifdef DEBUG
    else {
        if(verbose) {
            std::cout << "mode: Debug Quick Enabled (debug_quick)" << std::endl;
        }
        return OutputMode::DebugQuick;
    }
#endif
    return OutputMode::Debug;
}

LabJobType getJobTypeFromOpt(std::optional<std::string_view>& job_type_opt, LabJobType defaultType) {
    if(job_type_opt.has_value()) {
        auto& view = job_type_opt.value();
        if(view == "exe") {
            return LabJobType::Executable;
        } else if(view == "jit-exe") {
            return LabJobType::JITExecutable;
        } else if(view == "lib") {
            return LabJobType::Library;
        } else if(view == "2c") {
            return LabJobType::ToCTranslation;
        } else if(view == "2ch") {
            return LabJobType::ToChemicalTranslation;
        } else if(view == "inter") {
            return LabJobType::Intermediate;
        } else if(view == "proc") {
            return LabJobType::ProcessingOnly;
        } else {
            // TODO: unknown job type, probably an error
        }
    }
    return defaultType;
}

int compiler_main(int argc, char *argv[]) {

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
            CmdOption("include", CmdOptionType::MultiValued),
            CmdOption("build-dir", CmdOptionType::SingleValue),
            CmdOption("library", "l", CmdOptionType::MultiValued),
            CmdOption("cc", CmdOptionType::SubCommand),
            CmdOption("configure", CmdOptionType::SubCommand),
            CmdOption("linker", CmdOptionType::SubCommand),
            CmdOption("tcc-jit", CmdOptionType::SubCommand),
            CmdOption("ar", CmdOptionType::SubCommand),
            CmdOption("dlltool", CmdOptionType::SubCommand),
            CmdOption("ranlib", CmdOptionType::SubCommand),
            CmdOption("lib", CmdOptionType::SubCommand),
            CmdOption("mode", "m", CmdOptionType::SingleValue),
            CmdOption("plugin-mode", "pm", CmdOptionType::SingleValue),
            CmdOption("version", CmdOptionType::NoValue),
            CmdOption("help", CmdOptionType::NoValue),
            CmdOption("minify-c", "minify-c", CmdOptionType::NoValue),
            CmdOption("emit-c", "emit-c", CmdOptionType::NoValue),
            CmdOption("keepc", "keepc", CmdOptionType::NoValue),
            CmdOption("test", "test", CmdOptionType::NoValue),
            CmdOption("benchmark", "bm", CmdOptionType::NoValue),
            CmdOption("benchmark-files", "bm-files", CmdOptionType::NoValue),
            CmdOption("benchmark-modules", "bm-modules", CmdOptionType::NoValue),
            CmdOption("verbose", "v", CmdOptionType::NoValue),
            CmdOption("verbose-link", "vl", CmdOptionType::NoValue),
            CmdOption("", "g", CmdOptionType::NoValue),
            CmdOption("library", "l", CmdOptionType::MultiValued),
            CmdOption("ignore-errors", "ignore-errors", CmdOptionType::NoValue),
            CmdOption("lto", CmdOptionType::NoValue),
            CmdOption("assertions", CmdOptionType::NoValue),
            CmdOption("no-pie", "no-pie", CmdOptionType::NoValue),
            CmdOption("target", "t", CmdOptionType::SingleValue),
            CmdOption("jobs", "j", CmdOptionType::SingleValue),
            CmdOption("job-type", "jt", CmdOptionType::SingleValue),
            CmdOption("jit", "jit", CmdOptionType::NoValue),
            CmdOption("use-tcc", "use-tcc", CmdOptionType::NoValue),
            CmdOption("use-bc", "use-bitcode", CmdOptionType::NoValue),
            CmdOption("use-lld", "use-lld", CmdOptionType::NoValue),
            CmdOption("output", "o", CmdOptionType::SingleValue),
            CmdOption("resources", "res", CmdOptionType::SingleValue),
            CmdOption("ignore-extension", CmdOptionType::NoValue),
            CmdOption("no-cache", CmdOptionType::NoValue),
            CmdOption("frecompile-plugins", "frecompile-plugins", CmdOptionType::NoValue),
            CmdOption("out-ll", CmdOptionType::SingleValue),
            CmdOption("out-bc", CmdOptionType::SingleValue),
            CmdOption("out-obj", CmdOptionType::SingleValue),
            CmdOption("out-asm", CmdOptionType::SingleValue),
            CmdOption("out-bin", CmdOptionType::SingleValue),
            CmdOption("out-ll-all", CmdOptionType::NoValue),
            CmdOption("out-asm-all", CmdOptionType::NoValue),
            CmdOption("debug-ir", CmdOptionType::NoValue),
            CmdOption("", "c", CmdOptionType::NoValue),
            CmdOption("cbi-m", "cbi-m", CmdOptionType::MultiValued),
            CmdOption("", "fno-unwind-tables", CmdOptionType::NoValue),
            CmdOption("", "fno-asynchronous-unwind-tables", CmdOptionType::NoValue),
            CmdOption("mod", "", CmdOptionType::MultiValued),
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

    auto& jit_cmd_opt = options.cmd_opt("tcc-jit");
    if(argc > 1 && jit_cmd_opt.has_multi_value()) {
        auto mode = get_output_mode(options.option_new("mode", "m"), options.has_value("verbose", "v"));
        std::vector<char*> jit_args;
        auto span = jit_cmd_opt.get_multi_opt_values();
        for(auto& view : span) {
            jit_args.emplace_back(const_cast<char*>(view.data()));
        }
        auto compiler_exe_path = getExecutablePath();
        return LabBuildCompiler::tcc_run_invocation(compiler_exe_path.data(), args, mode, (int) jit_args.size(), jit_args.data());
    }

#ifdef COMPILER_BUILD
    auto llvm_tool = [](int argc, char** argv, CmdOptions& options, const std::string_view& option) -> int {
        auto& cmd_opt = options.cmd_opt(option);
        if(cmd_opt.has_multi_value() && !cmd_opt.get_multi_opt_values().empty()) {
            std::vector<chem::string> subc;
            subc.emplace_back(option);
            cmd_opt.get_multi_value_vec(subc);
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
    if(!cc_cmd_opt.get_multi_opt_values().empty()) {
        std::vector<chem::string> subc;
        subc.emplace_back(getExecutablePath());
        cc_cmd_opt.get_multi_value_vec(subc);
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

#ifdef DEBUG
    if(verbose) {
        std::cout << "source_dir: " << PROJECT_SOURCE_DIR << std::endl;
    }
#endif

    bool jit = options.has_value("jit", "jit");
    auto& output = options.option_new("output", "o");
    auto& res = options.option_new("resources", "res");
    auto& dash_c = options.option_new("", "c");

#ifdef COMPILER_BUILD

    auto get_resources_path = [&res, &argv]() -> std::string {
        auto resources_path = res.has_value() ? std::string(res.value()) : resources_path_rel_to_exe(getExecutablePath());
        if(resources_path.empty()) {
            std::cerr << rang::fg::yellow << "warning: " << rang::fg::reset;
            std::cerr << "couldn't locate resources path relative to compiler's executable" << std::endl;
        }
        return resources_path;
    };

#endif

    auto prepare_options = [&](LabBuildCompilerOptions* opts) -> void {
        opts->benchmark = options.has_value("benchmark", "bm");
        opts->benchmark_files = options.has_value("benchmark-files", "bm-files");
        opts->benchmark_modules = options.has_value("benchmark-modules", "bm-modules");
        opts->verbose = verbose;
        opts->verbose_link = options.has_value("verbose-link", "vl");
        opts->minify_c = options.has_value("minify-c");
        opts->emit_c = options.has_value("emit-c", "emit-c") || options.has_value("keepc", "keep-c");
        opts->debug_info = options.has_value("", "g") || (opts->out_mode == OutputMode::Debug || opts->out_mode == OutputMode::DebugComplete);
#ifdef COMPILER_BUILD
        opts->resources_path = get_resources_path();
        opts->use_tcc = options.has_value("use-tcc", "use-tcc");
        opts->use_lld = options.has_value("use-lld", "use-lld");
        opts->use_mod_obj_format = !options.has_value("use-bc", "use-bitcode");
#endif
        opts->ignore_errors = options.has_value("ignore-errors", "ignore-errors");
        auto mode_opt = options.option_new("plugin-mode", "pm");
        if(mode_opt.has_value()) {
            const auto mode = get_output_mode(mode_opt, false);
            opts->def_plugin_mode = mode;
        }
        if(options.has_value("no-cache")) {
            opts->is_caching_enabled = false;
        }
        if(options.has_value("frecompile-plugins")) {
            opts->force_recompile_plugins = true;
        }
        if(options.has_value("debug-ir")) {
            opts->debug_ir = true;
        }
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

    auto mode_opt = options.option_new("mode", "m");
    const auto mode = get_output_mode(mode_opt, verbose);

    // check manual jobs argument to calculate concurrency
    auto jobs_opt = options.option_new("jobs", "j");
    int threadCount;
    if(jobs_opt.has_value()) {
        auto num_value = parse_num(jobs_opt.value().data(), jobs_opt.value().size(), strtol);
        if(num_value.error.empty()) {
            threadCount = (int) num_value.result;
        } else {
            std::cerr << rang::fg::yellow << "warning: " << rang::fg::reset << "failed to parse `jobs` argument as a number '" << jobs_opt.value() << "'" << std::endl;
            threadCount = (int) std::thread::hardware_concurrency();
        }
    } else {
        threadCount = (int) std::thread::hardware_concurrency();
    }
    if(threadCount <= 0) threadCount = 1;

    auto build_dir_opt = options.option_new("build-dir", "b");
    // TODO: handle this parameter
    const auto is_testing_env = options.has_value("test");

    const auto is_lab_file = args[0].ends_with(".lab");
    const auto is_mod_file = args[0].ends_with(".mod");
    if(is_mod_file && output.has_value() && output.value().ends_with(".lab")) {
        // conversion of a .mod file to .lab file
        return LabBuildCompiler::translate_mod_file_to_lab(chem::string_view(args[0]), chem::string_view(output.value()));
    }
    // build a .lab file
    if(is_lab_file || is_mod_file) {

        auto compiler_exe_path = getExecutablePath();
        std::string build_dir = build_dir_opt.has_value() ? std::string(build_dir_opt.value()) : resolve_non_canon_parent_path(args[0], "build");
        LabBuildCompilerOptions compiler_opts(compiler_exe_path, target, std::move(build_dir), is64Bit);
        CompilerBinder binder(compiler_exe_path);
        LocationManager loc_man;
        LabBuildCompiler compiler(loc_man, binder, &compiler_opts, threadCount);
        compiler.set_cmd_options(&options);

        // Prepare compiler options
        compiler_opts.out_mode = mode;
        compiler_opts.def_out_mode = mode;
        prepare_options(&compiler_opts);

        // translate the build.lab to a c file (for debugging)
        if(output.has_value() && output.value().ends_with(".c")) {
            LabJob job(LabJobType::ToCTranslation, chem::string("[BuildLabTranslation]"), chem::string(output.value()), chem::string(compiler_opts.build_dir), mode);
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
            auto job_type_opt = options.option_new("job-type", "jt");
            LabJobType final_job_type = getJobTypeFromOpt(job_type_opt, LabJobType::Executable);
            LabJob final_job(final_job_type, chem::string("main"), std::move(outputPath), chem::string(compiler_opts.build_dir), mode);
            LabBuildContext::initialize_job(&final_job, &compiler_opts);
            const auto result = compiler.build_mod_file(context, args[0], &final_job);
            return result;
        }

    }

    // compilation
    auto compiler_exe_path = getExecutablePath();
    std::string build_dir = build_dir_opt.has_value() ? std::string(build_dir_opt.value()) : "./";
    LabBuildCompilerOptions compiler_opts(compiler_exe_path, target, std::move(build_dir), is64Bit);
    CompilerBinder binder(compiler_exe_path);
    LocationManager loc_man;
    LabBuildCompiler compiler(loc_man, binder, &compiler_opts, threadCount);
    compiler.set_cmd_options(&options);

    // set default compiler options
    // we disable cache (because its command line invocation)
    compiler_opts.is_caching_enabled = false;
    compiler_opts.out_mode = mode;
    compiler_opts.def_out_mode = mode;
    prepare_options(&compiler_opts);

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

    const auto defJobType = jit ? LabJobType::JITExecutable : LabJobType::Executable;
    auto job_type_opt = options.option_new("job-type", "jt");
    LabJob job(getJobTypeFromOpt(job_type_opt, defJobType), chem::string("a"), mode);
    job.mode = mode;
    if(!target.empty()) {
        // TODO: update the target data according to target triple
        job.target_triple.append(target);
    }

    std::vector<std::unique_ptr<LabModule>> dependencies;
    unsigned i = 0;
    for(auto& arg : args) {
        if(arg.ends_with(".c")) {
            auto index = std::to_string(i);
            auto dep = new LabModule(LabModuleType::CFile, chem::string(""), chem::string(""));
            dependencies.emplace_back(dep);
            dep->paths.emplace_back(arg);
            job.dependencies.emplace_back(dep);
        } else if(arg.ends_with(".o")) {
            auto index = std::to_string(i);
            auto dep = new LabModule(LabModuleType::ObjFile, chem::string(""), chem::string(""));
            dependencies.emplace_back(dep);
            dep->paths.emplace_back(arg);
            job.dependencies.emplace_back(dep);
        } else {
            module.paths.emplace_back(std::string(arg));
        }
        i++;
    }
    // add the module if paths is not empty
    if(!module.paths.empty()) {
        job.dependencies.emplace_back(&module);
    }

    set_options_for_main_job(options, job, module, dependencies);
    auto link_libs = options.data.find("library")->second.get_multi_opt_values();
    for(auto& lib : link_libs) {
        job.link_libs.emplace_back(chem::string::make_view(lib));
    }

    // checking if user requires ll, asm output at default location for the module
    const auto has_ll = options.has_value("out-ll-all");
    const auto has_asm = options.has_value("out-asm-all");
    if(has_ll || has_asm) {
        const auto mod_dir = resolve_rel_child_path_str(job.build_dir.to_view(), module.name.to_view());
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
    if(output.has_value()) {
        job.abs_path.append(std::string(output.value()));
    }
    auto return_int = compiler.do_job_allocating(&job);

    // delete object file which was linked
    if(!temporary_obj.empty()) {
        std::error_code ec;
        std::filesystem::remove(temporary_obj, ec);
        if(ec) {
            std::cerr << rang::fg::red << "error: couldn't delete temporary object file " << temporary_obj << " because " << ec.message() << rang::fg::reset << std::endl;
            return_int = 1;
        }
    }

    return return_int;

}