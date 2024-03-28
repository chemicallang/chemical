// Copyright (c) Qinetik 2024.

#include "IndexOperator.h"
#include "ast/base/ASTNode.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

// TODO isInBounds optimization, when we know that index is in bounds
llvm::Value* IndexOperator::elem_pointer(Codegen& gen, ASTNode* arr) {
    std::vector<llvm::Value*> idxList;
    auto type = arr->llvm_type(gen);
    if(type->isArrayTy()) {
        idxList.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    }
    idxList.push_back(value->llvm_value(gen));
    idxList.shrink_to_fit();
    return gen.builder->CreateGEP(type, arr->llvm_pointer(gen), idxList);;
}

llvm::Value * IndexOperator::llvm_pointer(Codegen &gen) {
    auto arr = resolve(gen);
    return elem_pointer(gen, arr);
}

llvm::Value * IndexOperator::llvm_value(Codegen &gen) {
    auto resolved = resolve(gen);
    return gen.builder->CreateLoad(resolved->llvm_elem_type(gen), elem_pointer(gen, resolved), "arr0");
}

#endif