// Copyright (c) Qinetik 2024.

#pragma once

#include "libtcc.h"
#include <string>

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

TCCState* compile_c_to_tcc_state(char* exe_path, char* program, const std::string& outputFileName, bool jit);

int compile_c_string(char* exe_path, char* program, const std::string& outputFileName, bool jit, bool benchmark);