// Copyright (c) Chemical Language Foundation 2025.

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

/**
 * this function creates a new tcc state and also sets up it's libraries and includes correctly
 */
TCCState* tcc_new_state(const char* exe_path, const char* debug_file_name);

/**
 * set the output type to jit for a tcc state, should be set right after creation of the tcc state
 */
bool tcc_set_output_for_jit(TCCState* s);

/**
 * should be set after creation, but always before compilation, this makes it so
 * that tcc generates debug code
 */
void tcc_set_debug_options(TCCState* s);

TCCState* setup_tcc_state(char* exe_path, const std::string& outputFileName, bool jit, bool debug);

TCCState* compile_c_to_tcc_state(char* exe_path, const char* program, const std::string& outputFileName, bool jit, bool debug);

/**
 * will redirect functions like malloc, free and memcpy to our compiler
 * so we can free user allocated pointers
 */
void prepare_tcc_state_for_jit(TCCState* state);

int compile_c_string(char* exe_path, const char* program, const std::string& outputFileName, bool jit, bool benchmark, bool debug);

std::optional<std::string> read_file_to_string(const char* file_path);

int compile_c_file(char* exe_path, const char* c_file_path, const std::string& outputFileName, bool jit, bool benchmark, bool debug);

int tcc_link_objects(char* exe_path, const std::string& outputFileName, std::vector<chem::string>& objects);