// Copyright (c) Qinetik 2024.

#pragma once

#include "compiler/lab/BackendContext.h"

class Codegen;

class LLVMBackendContext : public BackendContext {
public:

    Codegen* gen_ptr;

    LLVMBackendContext(Codegen* gen) : gen_ptr(gen){

    }

    void mem_copy(Value* lhs, Value* rhs) override;

};