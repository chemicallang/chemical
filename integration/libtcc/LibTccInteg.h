// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "libtcc.h"
#include <string>
#include <vector>
#include <optional>
#include "std/chem_string.h"
#include "TCCMode.h"

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

/**
 * this function creates a new tcc state and also sets up it's libraries and includes correctly
 */
TCCState* tcc_new_state(const char* exe_path, const char* debug_file_name, TCCMode mode);

TCCState* setup_tcc_state(char* exe_path, const std::string& outputFileName, bool jit, TCCMode mode);

TCCState* compile_c_to_tcc_state(char* exe_path, const char* program, const std::string& outputFileName, bool jit, TCCMode mode);

/**
 * will redirect functions like malloc, free and memcpy to our compiler
 * so we can free user allocated pointers
 */
void prepare_tcc_state_for_jit(TCCState* state);

int compile_c_string(char* exe_path, const char* program, const std::string& outputFileName, bool jit, bool benchmark, TCCMode mode);

std::optional<std::string> read_file_to_string(const char* file_path);

int compile_c_file(char* exe_path, const char* c_file_path, const std::string& outputFileName, bool jit, bool benchmark, TCCMode mode);

int tcc_link_objects(char* exe_path, const std::string& outputFileName, std::vector<chem::string>& objects, TCCMode mode);