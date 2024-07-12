// Copyright (c) Qinetik 2024.

#include "LibTccInteg.h"
#include <iostream>
#include <vector>
#include "utils/Benchmark.h"
#include "utils/PathUtils.h"

void handle_tcc_error(void *opaque, const char *msg){
    std::cout << "[Tcc] " << msg << " in " << ((char*) opaque) << std::endl;
}

TCCState* compile_c_to_tcc_state(char* exe_path, const char* program, const std::string& outputFileName, bool jit, bool debug) {

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

    int outputType = TCC_OUTPUT_EXE;
    if(jit || outputFileName.empty()) {
        outputType = TCC_OUTPUT_MEMORY;
    } else {
        if(outputFileName.ends_with(".exe")) {
            outputType = TCC_OUTPUT_EXE;
        } else if(outputFileName.ends_with(".o")) {
            outputType = TCC_OUTPUT_OBJ;
        } else if(outputFileName.ends_with(".dll") || outputFileName.ends_with(".so")) {
            outputType = TCC_OUTPUT_DLL;
        }
    }

    if(debug) {
        // generates slower code in debug versions, but allows proper debugging
        tcc_set_options(s, "-g -b -bt 4");
        tcc_set_backtrace_func(s, nullptr, [](void *udata, void *pc, const char *file, int line, const char *func, const char *msg) {
               std::cerr << "[Tcc] error '" << msg << "' in runtime function " << func << " in file "
                         << file << ':' << line << std::endl;
               return 1;
       });
    }

    result = tcc_set_output_type(s, outputType);
    if(result == -1) {
        std::cerr << "Couldn't set tcc output type" << std::endl;
        return nullptr;
    }

    if (program != nullptr && tcc_compile_string(s, program) == -1) {
        std::cerr << "Couldn't compile the program" << std::endl;
        return nullptr;
    }

    // we will use our heap context
    // this way c and our program and can free or allocate for each other
    if(jit) {
        tcc_undefine_symbol(s, "malloc");
        tcc_undefine_symbol(s, "realloc");
        tcc_undefine_symbol(s, "free");
        tcc_undefine_symbol(s, "memcpy");
        tcc_add_symbol(s, "malloc", (void*) malloc);
        tcc_add_symbol(s, "realloc", (void*) realloc);
        tcc_add_symbol(s, "free", (void*) free);
        tcc_add_symbol(s, "memcpy", (void*) memcpy);
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

int tcc_link_objects(char* exe_path, const std::string& outputFileName, std::vector<chem::string>& objects) {

    auto s = compile_c_to_tcc_state(exe_path, nullptr, exe_path, false, false);
    if(!s) {
        return 1;
    }

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
    tcc_output_file(s, outputFileName.data());

    return 0;

}