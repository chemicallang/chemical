// Copyright (c) Qinetik 2024.

#pragma once

#include "compiler/lab/BackendContext.h"

class Codegen;

class LLVMBackendContext : public BackendContext {
public:

    Codegen* gen;

    LLVMBackendContext(Codegen* gen) : gen(gen){

    }

};