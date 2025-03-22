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

int tcc_backtrace_fn_handler(void *udata, void *pc, const char *file, int line, const char *func, const char *msg) {
    std::cerr << "[tcc] " << rang::fg::red << "error: '" << rang::fg::reset << msg << "' in runtime function " << func << " at " << file << ':' << line << std::endl;
    return 1;
}

TCCState* tcc_new_state(const char* exe_path, const char* debug_file_name) {

    // creating a tcc state
    auto s = tcc_new();
    if (!s) {
        fprintf(stderr, "Could not create tcc state\n");
        return nullptr;
    }

    // set custom error/warning printer
    tcc_set_error_func(s, (void*) debug_file_name, handle_tcc_error);

    // the tcc dir contains everything tcc needs present relative to our compiler executable
    auto tcc_dir = resolve_non_canon_parent_path(exe_path, "packages/tcc");

    // TODO check if tcc dir is not present and error out appropriately

    // adding tcc include paths (which should be present relative to our compiler executable
    auto include_dir = resolve_rel_child_path_str(tcc_dir, "include");
    const auto includeRes = tcc_add_include_path(s, include_dir.data());;
    if (includeRes == -1) {
        std::cerr << rang::fg::red << "error: " << rang::fg::reset;
        std::cerr << "couldn't include tcc include package" << std::endl;
        return nullptr;
    }

    // adding tcc library path, pre
    auto lib_dir = resolve_rel_child_path_str(tcc_dir, "lib");
    const auto addRes = tcc_add_library_path(s, lib_dir.data());
    if (addRes == -1) {
        std::cerr << rang::fg::red << "error: " << rang::fg::reset;
        std::cerr << "couldn't add tcc library package" << std::endl;
        return nullptr;
    }

    return s;

}

bool tcc_set_output_for_jit(TCCState* s) {
    const auto outputRes = tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    if (outputRes == -1) {
        std::cerr << rang::fg::red << "error: " << rang::fg::reset << "couldn't set tcc output type" << std::endl;
        return false;
    }
    return true;
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

void tcc_set_debug_options(TCCState* s) {
    // generates slower code in debug versions, but allows proper debugging
    // it would be helpful to set -b but that's not working on my system
    // -b
    // Generate additional support code to check memory allocations and array/pointer bounds. -g is implied. Note that the generated code is slower and bigger in this case.
    // Note: -b is only available on i386 when using libtcc for the moment.
    // -bt N Display N callers in stack traces. This is useful with -g or -b.
    tcc_set_options(s, "-g -bt 4");
    tcc_set_backtrace_func(s, nullptr, tcc_backtrace_fn_handler);
}

TCCState* setup_tcc_state(char* exe_path, const std::string& outputFileName, bool jit, bool debug) {

    // creating a tcc state
    TCCState *s;
    s = tcc_new();
    if (!s) {
        fprintf(stderr, "Could not create tcc state\n");
        exit(1);
    }

    /* set custom error/warning printer */
    tcc_set_error_func(s, (char *) outputFileName.data(), handle_tcc_error);

    int result;

    auto tcc_dir = resolve_non_canon_parent_path(exe_path, "packages/tcc");
    auto include_dir = resolve_rel_child_path_str(tcc_dir, "include");
    auto lib_dir = resolve_rel_child_path_str(tcc_dir, "lib");
    result = tcc_add_include_path(s, include_dir.data());;
    if (result == -1) {
        std::cerr << rang::fg::red << "error: " << rang::fg::reset;
        std::cerr << "couldn't include tcc include package" << std::endl;
        return nullptr;
    }

    result = tcc_add_library_path(s, lib_dir.data());
    if (result == -1) {
        std::cerr << rang::fg::red << "error: " << rang::fg::reset;
        std::cerr << "couldn't add tcc library package" << std::endl;
        return nullptr;
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

    if (debug) {
        // generates slower code in debug versions, but allows proper debugging
        // it would be helpful to set -b but that's not working on my system
        // -b
        // Generate additional support code to check memory allocations and array/pointer bounds. -g is implied. Note that the generated code is slower and bigger in this case.
        // Note: -b is only available on i386 when using libtcc for the moment.
        // -bt N Display N callers in stack traces. This is useful with -g or -b.
        tcc_set_options(s, "-g -bt 4");
        tcc_set_backtrace_func(s, nullptr, [](void *udata, void *pc, const char *file, int line, const char *func,
                                              const char *msg) {
            std::cerr << "[Tcc] error '" << msg << "' in runtime function " << func << " in file "
                      << file << ':' << line << std::endl;
            return 1;
        });
    }

    result = tcc_set_output_type(s, outputType);
    if (result == -1) {
        std::cerr << "Couldn't set tcc output type" << std::endl;
        return nullptr;
    }

    return s;

}

void prepare_tcc_state_for_jit(TCCState* s) {
    tcc_undefine_symbol(s, "malloc");
    tcc_undefine_symbol(s, "realloc");
    tcc_undefine_symbol(s, "free");
    tcc_undefine_symbol(s, "memcpy");
    tcc_add_symbol(s, "malloc", (void*) malloc);
    tcc_add_symbol(s, "realloc", (void*) realloc);
    tcc_add_symbol(s, "free", (void*) free);
    tcc_add_symbol(s, "memcpy", (void*) memcpy);
}

TCCState* compile_c_to_tcc_state(char* exe_path, const char* program, const std::string& outputFileName, bool jit, bool debug) {

    auto s = setup_tcc_state(exe_path, outputFileName, jit, debug);

    if (tcc_compile_string(s, program) == -1) {
        std::cerr << "Couldn't compile the program" << std::endl;
        return nullptr;
    }

    // we will use our heap context
    // this way c and our program and can free or allocate for each other
    if(jit) {
        prepare_tcc_state_for_jit(s);
    }

    return s;

}

int compile_c_string(char* exe_path, const char* program, const std::string& outputFileName, bool jit, bool benchmark, bool debug) {

    BenchmarkResults results{};
    if(benchmark) {
        results.benchmark_begin();
    }

    auto s = compile_c_to_tcc_state(exe_path, program, outputFileName, jit, debug);
    if(!s) {
        return 1;
    }

    if(jit) {
        std::vector<char*> argv;
        argv.emplace_back(exe_path);
        tcc_run(s, 1, argv.data());
    } else {
        tcc_output_file(s, outputFileName.data());
    }

    /* delete the state */
    tcc_delete(s);

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

int compile_c_file(char* exe_path, const char* c_file_path, const std::string& outputFileName, bool jit, bool benchmark, bool debug) {
    auto read = read_file_to_string(c_file_path);
    if(read.has_value()) {
        return compile_c_string(exe_path, read.value().data(), outputFileName, jit, benchmark, debug);
    } else {
        std::cerr << "couldn't open c file at " << c_file_path << " for compilation" << std::endl;
        return 1;
    }
}

int tcc_link_objects(char* exe_path, const std::string& outputFileName, std::vector<chem::string>& objects) {

    // creating a new tcc state
    const auto s = tcc_new_state(exe_path, outputFileName.data());
    if(!s) return 1;

    // set output according to extension
    tcc_set_output_for_extension(s, outputFileName);

    // auto delete state
    TCCDeletor del_auto(s);

    // adding object files
    std::string command;
    for(auto& obj : objects) {
        if(tcc_add_file(s, obj.data()) == -1) {
            std::cerr << "[Tcc] couldn't link " << obj << std::endl;
            return 1;
        }
    }

    // output file
    if(tcc_output_file(s, outputFileName.data()) == -1) {
        std::cerr << "[Tcc] couldn't output file " << outputFileName << std::endl;
        return 1;
    }

    return 0;

}