// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "SanitizerOptions.h"

class CodegenEmitterOptions {
public:
    bool assertions_on = false;
    bool is_debug = true;
    bool is_small = false;
    bool time_report = true;
    SanitizerType sanitizers = SanitizerType::None;
    bool lto = false;
    bool debug_ir = false;
    const char* asm_path = nullptr;
    const char* obj_path = nullptr;
    const char* ir_path = nullptr;
    const char* bitcode_path = nullptr;
};