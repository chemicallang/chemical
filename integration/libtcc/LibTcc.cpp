// Copyright (c) Chemical Language Foundation 2025.

#include "LibTccInteg.h"
#include <iostream>
#include <vector>
#include "utils/Benchmark.h"
#include "utils/PathUtils.h"
#include "rang.hpp"
#include <fstream>
#include <sstream>

void handle_tcc_error(void *opaque, const char *msg){
    std::cout << rang::fg::red << msg << " in " << ((char*) opaque) << rang::fg::reset << std::endl;
}

TCCState* tcc_new_state(const char* exe_path, const char* debug_file_name, TCCMode mode) {

    // creating a tcc state
    auto s = tcc_new();
    if (!s) {
        std::cerr << rang::fg::red << "error: " << rang::fg::reset;
        std::cerr << "couldn't create a new tcc state" << std::endl;
        return nullptr;
    }

    // set custom error/warning printer
    tcc_set_error_func(s, (void*) debug_file_name, handle_tcc_error);

#ifdef DEBUG

    const auto source_dir_path = PROJECT_SOURCE_DIR;

    // the tcc dir contains everything tcc needs present relative to our compiler executable
    const auto tcc_dir = resolve_rel_child_path_str(source_dir_path, "lib/tcc");

#else

    // the tcc dir contains everything tcc needs present relative to our compiler executable
    auto tcc_dir = resolve_non_canon_parent_path(exe_path, "packages/tcc");

#endif

    // adding tcc include paths (which should be present relative to our compiler executable
    auto include_dir = resolve_rel_child_path_str(tcc_dir, "include");
    const auto includeRes = tcc_add_include_path(s, include_dir.data());;
    if (includeRes == -1) {
        std::cerr << rang::fg::red << "error: " << rang::fg::reset;
        std::cerr << "couldn't include tcc include package at '" << include_dir << '\'' << std::endl;
        return nullptr;
    }

    // adding tcc library path, pre
    auto lib_dir = resolve_rel_child_path_str(tcc_dir, "lib");
    const auto addRes = tcc_add_library_path(s, lib_dir.data());
    if (addRes == -1) {
        std::cerr << rang::fg::red << "error: " << rang::fg::reset;
        std::cerr << "couldn't add tcc library package at '" << lib_dir << '\'' << std::endl;
        return nullptr;
    }

    // -b
    // Generate additional support code to check memory allocations and array/pointer bounds. -g is implied. Note that the generated code is slower and bigger in this case.
    // Note: -b is only available on i386 when using libtcc for the moment.
    // -bt N Display N callers in stack traces. This is useful with -g or -b.
    switch(mode) {
        case TCCMode::None:
            break;
        case TCCMode::Debug:
            tcc_set_options(s, "-g -bt 25");
            break;
        case TCCMode::DebugComplete:
            tcc_set_options(s, "-g -bt 25 -b");
            break;
    }

    return s;

}

bool tcc_set_output_for_extension(TCCState* s, const std::string& outputFileName) {
    int outputType = TCC_OUTPUT_EXE;
    if (!outputFileName.empty()) {
        if (outputFileName.ends_with(".exe")) {
            outputType = TCC_OUTPUT_EXE;
        } else if (outputFileName.ends_with(".o")) {
            outputType = TCC_OUTPUT_OBJ;
        } else if (outputFileName.ends_with(".dll") || outputFileName.ends_with(".so")) {
            outputType = TCC_OUTPUT_DLL;
        }
    }
    const auto outputRes = tcc_set_output_type(s, outputType);
    if (outputRes == -1) {
        std::cerr << rang::fg::red << "error: " << rang::fg::reset << "couldn't set tcc output type" << std::endl;
        return false;
    }
    return true;
}

int backtrace_handler(void *udata, void *pc, const char *file, int line, const char *func, const char *msg) {

    if(!file && !func && !msg) return 0;

    if (msg) {
        // msg is only given on the first
        std::cerr << "\n=== RUNTIME CRASH ===\n";
        std::cerr << "Stack trace:\n";
    }

    std::cerr << "  at ";

    if (file) {
        std::cerr << file << ':' << line << ": ";
    }

    std::cerr << "in ";

    if (func) {
        std::cerr << '\'' << func << '\'';
    } else {
        std::cerr << "<unknown function>";
    }

    if (msg) {
        std::cerr << " - error: '" << msg << '\'';
    }

    std::cerr << '\n';
    return 1;
}

TCCState* setup_tcc_state(char* exe_path, const std::string& outputFileName, bool jit, TCCMode mode) {

    const auto s = tcc_new_state(exe_path, outputFileName.c_str(), mode);
    if(s == nullptr) {
        return nullptr;
    }

    if (jit) {
        tcc_set_backtrace_func(s, nullptr, backtrace_handler);
    }

    int outputType = TCC_OUTPUT_EXE;
    if (jit || outputFileName.empty()) {
        outputType = TCC_OUTPUT_MEMORY;
    } else {
        if (outputFileName.ends_with(".exe")) {
            outputType = TCC_OUTPUT_EXE;
        } else if (outputFileName.ends_with(".o")) {
            outputType = TCC_OUTPUT_OBJ;
        } else if (outputFileName.ends_with(".dll") || outputFileName.ends_with(".so")) {
            outputType = TCC_OUTPUT_DLL;
        }
    }

    const auto result = tcc_set_output_type(s, outputType);
    if (result == -1) {
        std::cerr << rang::fg::red << "error: " << rang::fg::reset;
        std::cerr << "couldn't set tcc output type" << std::endl;
        tcc_delete(s);
        return nullptr;
    }

    return s;

}

void prepare_tcc_state_for_jit(TCCState* s) {
    tcc_undefine_symbol(s, "malloc");
    tcc_undefine_symbol(s, "realloc");
    tcc_undefine_symbol(s, "free");
    tcc_undefine_symbol(s, "memcpy");
    tcc_undefine_symbol(s, "memmove");
    tcc_add_symbol(s, "malloc", (void*) malloc);
    tcc_add_symbol(s, "realloc", (void*) realloc);
    tcc_add_symbol(s, "free", (void*) free);
    tcc_add_symbol(s, "memcpy", (void*) memcpy);
    tcc_add_symbol(s, "memmove", (void*) memmove);
}

TCCState* compile_c_to_tcc_state(char* exe_path, const char* program, const std::string& outputFileName, bool jit, TCCMode mode) {

    auto s = setup_tcc_state(exe_path, outputFileName, jit, mode);
    if(!s) {
        return nullptr;
    }

    if (tcc_compile_string(s, program) == -1) {
        std::cerr << rang::fg::red << "error: " << rang::fg::reset;
        std::cerr << "couldn't compile the program" << std::endl;
        tcc_delete(s);
        return nullptr;
    }

    // we will use our heap context
    // this way c and our program and can free or allocate for each other
    if(jit) {
        prepare_tcc_state_for_jit(s);
    }

    return s;

}

int compile_c_string(char* exe_path, const char* program, const std::string& outputFileName, bool jit, bool benchmark, TCCMode mode) {

    BenchmarkResults results{};
    if(benchmark) {
        results.benchmark_begin();
    }

    // compile c to tcc state
    auto s = compile_c_to_tcc_state(exe_path, program, outputFileName, jit, mode);
    if(!s) {
        return 1;
    }

    // output the file
    const auto out_res = tcc_output_file(s, outputFileName.data());
    if (out_res == -1) {
        std::cerr << rang::fg::red << "error: " << rang::fg::reset;
        std::cerr << "couldn't output file '" << outputFileName << '\'' << std::endl;
        tcc_delete(s);
        return -1;
    }

    // delete the state
    tcc_delete(s);

    // end the benchmarks
    if(benchmark) {
        results.benchmark_end();
        std::cout << "[Tcc] " << results.representation() << std::endl;
    }

    return 0;

}

std::optional<std::string> read_file_to_string(const char* file_path) {
    std::ifstream input;
    input.open(file_path);
    if (!input.is_open()) {
        return std::nullopt;
    }
    std::stringstream buffer;
    buffer << input.rdbuf();
    input.close();
    return buffer.str();
}

int compile_c_file(char* exe_path, const char* c_file_path, const std::string& outputFileName, bool jit, bool benchmark, TCCMode mode) {
    auto read = read_file_to_string(c_file_path);
    if(read.has_value()) {
        return compile_c_string(exe_path, read.value().data(), outputFileName, jit, benchmark, mode);
    } else {
        std::cerr << "couldn't open c file at " << c_file_path << " for compilation" << std::endl;
        return 1;
    }
}

int tcc_link_objects(
    char* exe_path,
    const std::string& outputFileName,
    std::vector<chem::string>& objects,
    std::vector<chem::string>& link_libs,
    TCCMode mode
) {

    // creating a new tcc state
    const auto s = tcc_new_state(exe_path, outputFileName.data(), mode);
    if(!s) return 1;

    // set output according to extension
    tcc_set_output_for_extension(s, outputFileName);

    // auto delete state
    TCCDeletor del_auto(s);

    // adding object files
    std::string command;
    for(auto& obj : objects) {
        if(tcc_add_file(s, obj.data()) == -1) {
            std::cerr << "[Tcc] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't add object '" << obj << "'" << std::endl;
            return 1;
        }
    }

    for(auto& linkLib : link_libs) {
        if(tcc_add_library(s, linkLib.data()) == -1) {
            std::cerr << "[Tcc] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't link library '" << linkLib << "'" << std::endl;
        }
    }

    // output file
    if(tcc_output_file(s, outputFileName.data()) == -1) {
        std::cerr << "[Tcc] couldn't output file " << outputFileName << std::endl;
        return 1;
    }

    return 0;

}