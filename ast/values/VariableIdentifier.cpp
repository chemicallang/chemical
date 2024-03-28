// Copyright (c) Qinetik 2024.

#include "VariableIdentifier.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Value *VariableIdentifier::arg_value(Codegen &gen, ASTNode *node) {
    auto param = node->as_parameter();
    if (param != nullptr && gen.current_function != nullptr) {
        for (const auto &arg: gen.current_function->args()) {
            if (arg.getArgNo() == param->index) {
                return (llvm::Value *) &arg;
            }
//                else {
//                    gen.error("no mismatch" + std::to_string(arg.getArgNo()) + " " + std::to_string(param->index));
//                }
        }
    }
//        else {
//            gen.error("param or function missing");
//        }
    return nullptr;
}

llvm::AllocaInst *VariableIdentifier::llvm_alloca(Codegen &gen) {
    auto found = gen.allocated.find(value);
    if (found == gen.allocated.end()) {
        gen.error("llvm_alloca called on variable identifier, couldn't locate the identifier : " + value);
        return nullptr;
    } else {
        return found->second;
    }
}

llvm::Value *VariableIdentifier::llvm_pointer(Codegen &gen) {
    return llvm_alloca(gen);
}

llvm::Value *VariableIdentifier::llvm_value(Codegen &gen) {
    auto resolved = resolve(gen);
    auto argVal = arg_value(gen, resolved);
    if (argVal != nullptr) {
        return argVal;
    }
    auto v = llvm_pointer(gen);
    return gen.builder->CreateLoad(resolved->llvm_type(gen), v, value);
}

#endif