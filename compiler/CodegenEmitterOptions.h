// Copyright (c) Qinetik 2024.

#pragma once

class CodegenEmitterOptions {
public:
    bool assertions_on = false;
    bool is_debug = true;
    bool is_small = false;
    bool time_report = true;
    bool tsan = false;
    bool lto = false;
    char* asm_path = nullptr;
    char* obj_path = nullptr;
    char* ir_path = nullptr;
    char* bitcode_path = nullptr;
};