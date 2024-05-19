// Copyright (c) Qinetik 2024.

#include "TryCatch.h"
#include "ast/values/FunctionCall.h"

TryCatch::TryCatch(
        std::unique_ptr<FunctionCall> tryCall,
        catch_var_type catchVar,
        std::optional<Scope> catchScope
) : tryCall(std::move(tryCall)), catchVar(std::move(catchVar)), catchScope(std::move(catchScope)){

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
        gen.error("no catch body given in try catch, which is not supported yet");
    }
    gen.SetInsertPoint(normal);
}

#endif

void TryCatch::accept(Visitor *visitor) {
    visitor->visit(this);
}

void TryCatch::declare_and_link(SymbolResolver &linker) {
    tryCall->link(linker);
    if(catchScope.has_value()) {
        catchScope->declare_and_link(linker);
    }
}

std::string TryCatch::representation() const {
    std::string rep("try " + tryCall->representation());
    if(catchScope.has_value()) {
        rep.append(" catch ");
        if(catchVar.has_value()) {
            rep.append("(" + catchVar.value().first + ':' + catchVar.value().second->representation() + ") ");
        }
        rep.append("{\n");
        rep.append(catchScope.value().representation());
        rep.append("\n}");
    }
    return rep;
}