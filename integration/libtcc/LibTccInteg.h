// Copyright (c) Qinetik 2024.

#pragma once

#include "libtcc.h"
#include <string>
#include <vector>
#include <optional>
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

std::optional<std::string> read_file_to_string(const char* file_path);

int compile_c_file(char* exe_path, const char* c_file_path, const std::string& outputFileName, bool jit, bool benchmark, bool debug);

int tcc_link_objects(char* exe_path, const std::string& outputFileName, std::vector<chem::string>& objects);