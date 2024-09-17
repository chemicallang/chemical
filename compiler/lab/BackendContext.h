// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

/**
 * a very simple, pure virtual class that is implemented by different backends to
 * provide functionality
 * example: this can be used to generate code, based on user's compile time
 * function calls, for example user calls memcpy using std::mem::cpy <-- C backend will write normal
 * memcpy (maybe shallow struct copy or call to std lib's memcpy) and LLVM backend will write a normal
 * llvm memcpy intrinsic function call
 * All this happens through this virtual class
 */
class BackendContext {
public:

    virtual void mem_copy(Value* lhs, Value* rhs) = 0;

};