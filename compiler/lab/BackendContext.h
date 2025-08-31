// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>

enum class CompilerFeatureKind : int {
    Float128 = 0
};

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

    /**
     * forgets, meaning no drop will be called on the given node
     * @return true if forgotten successfully, false otherwise
     */
    virtual bool forget(ASTNode* node) = 0;

    /**
     * performs a mem cpy on lhs and rhs using the backend
     */
    virtual void mem_copy(Value* lhs, Value* rhs) = 0;

    /**
     * is the following feature supported inside the compiler ?
     */
    virtual bool supports(CompilerFeatureKind kind) = 0;

    /**
     * destruct the call site
     */
    virtual void destruct_call_site(SourceLocation location) = 0;

};