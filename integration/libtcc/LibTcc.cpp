// Copyright (c) Qinetik 2024.

#include "LibTccInteg.h"
#include <iostream>
#include <vector>
#include "utils/Benchmark.h"
#include "utils/PathUtils.h"

void handle_tcc_error(void *opaque, const char *msg){
    std::cout << "[2cTcc] " << msg << " compiling to " << ((char*) opaque) << std::endl;
}

TCCState* compile_c_to_tcc_state(char* exe_path, const char* program, const std::string& outputFileName, bool jit) {

    // creating a tcc state
    TCCState *s;
    s = tcc_new();
    if (!s) {
        fprintf(stderr, "Could not create tcc state\n");
        exit(1);
    }

    /* set custom error/warning printer */
    tcc_set_error_func(s, (char*) outputFileName.data(), handle_tcc_error);

    int result;

    auto tcc_dir = resolve_non_canon_parent_path(exe_path, "packages/tcc");
    auto include_dir = resolve_rel_child_path_str(tcc_dir, "include");
    auto lib_dir = resolve_rel_child_path_str(tcc_dir, "lib");
    result = tcc_add_include_path(s, include_dir.data());;
    if(result == -1) {
        std::cerr << "[Compiler] Couldn't include tcc include package" << std::endl;
        return nullptr;
    }

    result = tcc_add_library_path(s, lib_dir.data());
    if(result == -1) {
        std::cerr << "[Compiler] Couldn't add tcc library package" << std::endl;
        return nullptr;
    }

    int outputType = TCC_OUTPUT_MEMORY;
    if(!jit && !outputFileName.empty()){
        if(outputFileName.ends_with(".exe")) {
            outputType = TCC_OUTPUT_EXE;
        } else if(outputFileName.ends_with(".o")) {
            outputType = TCC_OUTPUT_OBJ;
        } else if(outputFileName.ends_with(".dll")) {
            outputType = TCC_OUTPUT_DLL;
        }
    }
    result = tcc_set_output_type(s, outputType);
    if(result == -1) {
        std::cerr << "Couldn't set tcc output type" << std::endl;
        return nullptr;
    }

    if (tcc_compile_string(s, program) == -1) {
        std::cerr << "Couldn't compile the program" << std::endl;
        return nullptr;
    }

    return s;

}

int compile_c_string(char* exe_path, const char* program, const std::string& outputFileName, bool jit, bool benchmark) {

    BenchmarkResults results{};
    if(benchmark) {
        results.benchmark_begin();
    }

    auto s = compile_c_to_tcc_state(exe_path, program, outputFileName, jit);
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
        std::cout << "[Compiler] " << results.representation() << std::endl;
    }

    return 0;

}
