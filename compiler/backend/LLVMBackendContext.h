// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "compiler/lab/BackendContext.h"

class Codegen;

class LLVMBackendContext : public BackendContext {
public:

    Codegen* gen_ptr;

    LLVMBackendContext(Codegen* gen) : gen_ptr(gen){

    }

    bool forget(ASTNode* node) final;

    void mem_copy(Value* lhs, Value* rhs) final;

    bool supports(CompilerFeatureKind kind) final {
        return true;
    }

    void destruct_call_site(SourceLocation location) final;

};