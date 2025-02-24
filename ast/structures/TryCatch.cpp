// Copyright (c) Chemical Language Foundation 2025.

#include "TryCatch.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/LinkedType.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void TryCatch::code_gen(Codegen &gen) {
    auto unwind = llvm::BasicBlock::Create(*gen.ctx, "unw", gen.current_function);
    auto normal = llvm::BasicBlock::Create(*gen.ctx, "norm", gen.current_function);
    tryCall->llvm_invoke(gen, normal, unwind);
    gen.SetInsertPoint(unwind);
    if(catchScope.has_value()) {
        catchScope.value().code_gen(gen);
    } else {
        gen.error("no catch body given in try catch, which is not supported yet", this);
    }
    gen.SetInsertPoint(normal);
}

#endif

void TryCatch::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    tryCall->link(linker, (Value*&) tryCall);
    if(catchScope.has_value()) {
        catchScope->link_sequentially(linker);
    }
}