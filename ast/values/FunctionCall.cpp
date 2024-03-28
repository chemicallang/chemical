// Copyright (c) Qinetik 2024.

#include "FunctionCall.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Value *FunctionCall::llvm_value(Codegen &gen) {
    auto fn = gen.module->getFunction(name);
    if (fn == nullptr) {
        gen.error("function with name " + name + " does not exist");
        return nullptr;
    }
    std::vector<llvm::Value *> args(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        args[i] = values[i]->llvm_value(gen);
        // Ensure proper type promotion for float values passed to printf
        if (fn->isVarArg() && llvm::isa<llvm::ConstantFP>(args[i]) &&
            args[i]->getType() != llvm::Type::getDoubleTy(*gen.ctx)) {
            args[i] = gen.builder->CreateFPExt(args[i], llvm::Type::getDoubleTy(*gen.ctx));
        }
    }
    return gen.builder->CreateCall(fn, args);
}

void FunctionCall::code_gen(Codegen &gen) {
    llvm_value(gen);
}

#endif