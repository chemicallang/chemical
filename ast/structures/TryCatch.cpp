// Copyright (c) Qinetik 2024.

#include "TryCatch.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/LinkedType.h"

TryCatch::TryCatch(
        std::unique_ptr<FunctionCall> tryCall,
        catch_var_type catchVar,
        std::optional<Scope> catchScope,
        ASTNode* parent_node,
        CSTToken* token
) : tryCall(std::move(tryCall)), catchVar(std::move(catchVar)), catchScope(std::move(catchScope)), parent_node(parent_node), token(token) {

}

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

void TryCatch::accept(Visitor *visitor) {
    visitor->visit(this);
}

void TryCatch::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    tryCall->link(linker, (Value*&) tryCall);
    if(catchScope.has_value()) {
        catchScope->link_sequentially(linker);
    }
}