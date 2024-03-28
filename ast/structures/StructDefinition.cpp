// Copyright (c) Qinetik 2024.

#include "StructDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

void StructDefinition::code_gen(Codegen &gen) {
    llvm::StructType::create(*gen.ctx, elements_type(gen), name);
}

#endif