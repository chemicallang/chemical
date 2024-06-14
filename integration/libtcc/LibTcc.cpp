// Copyright (c) Qinetik 2024.

#include "LibTccInteg.h"
#include "libtcc.h"
#include <iostream>
#include <vector>
#include "utils/Benchmark.h"
#include "utils/PathUtils.h"

void handle_tcc_error(void *opaque, const char *msg){
    std::cout << "[Compiler] Error Compiling : " << msg << std::endl;
}

int compile_c_string(char* exe_path, char* program, const std::string& outputFileName, bool jit, bool benchmark) {

    BenchmarkResults results{};
    if(benchmark) {
        results.benchmark_begin();
    }

    // creating a tcc state
    TCCState *s;
    s = tcc_new();
    if (!s) {
        fprintf(stderr, "Could not create tcc state\n");
        exit(1);
    }

    /* set custom error/warning printer */
    tcc_set_error_func(s, stderr, handle_tcc_error);

    int result;

    auto tcc_dir = resolve_non_canon_parent_path(exe_path, "packages/tcc");
    auto include_dir = resolve_rel_child_path_str(tcc_dir, "include");
    auto lib_dir = resolve_rel_child_path_str(tcc_dir, "lib");
    result = tcc_add_include_path(s, include_dir.data());;
    if(result == -1) {
        std::cerr << "[Compiler] Couldn't include tcc include package" << std::endl;
        return 1;
    }

    result = tcc_add_library_path(s, lib_dir.data());
    if(result == -1) {
        std::cerr << "[Compiler] Couldn't add tcc library package" << std::endl;
        return 1;
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
        return 1;
    }

    if (tcc_compile_string(s, program) == -1) {
        std::cerr << "Couldn't compile the program" << std::endl;
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

