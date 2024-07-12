// Copyright (c) Qinetik 2024.

#pragma once

#include "libtcc.h"
#include <string>
#include <vector>
#include "std/chem_string.h"

class TCCDeletor {
public:
    TCCState* state;
    TCCDeletor(TCCState* state) : state(state){
        // nothing here
    }
    ~TCCDeletor() {
        tcc_delete(state);
    }
};

TCCState* compile_c_to_tcc_state(char* exe_path, const char* program, const std::string& outputFileName, bool jit, bool debug);

int compile_c_string(char* exe_path, const char* program, const std::string& outputFileName, bool jit, bool benchmark, bool debug);

int tcc_link_objects(char* exe_path, const std::string& outputFileName, std::vector<std::string>& objects);