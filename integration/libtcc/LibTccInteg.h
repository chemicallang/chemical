// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

int compile_c_string(char* exe_path, char* program, const std::string& outputFileName, bool jit, bool benchmark);