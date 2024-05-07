// Copyright (c) Qinetik 2024.

#include "FunctionType.h"
#include "ast/base/Value.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::FunctionType *FunctionType::llvm_func_type(Codegen &gen) {
    std::vector<llvm::Type *> paramTypes;
    if(isCapturing) {
        paramTypes.emplace_back(gen.builder->getPtrTy());
    }
    for (auto &param: params) {
        paramTypes.emplace_back(param->llvm_type(gen));
    }
    return llvm::FunctionType::get(returnType->llvm_type(gen), paramTypes, isVariadic);
}

llvm::Type *FunctionType::llvm_type(Codegen &gen) const {
    return gen.builder->getPtrTy();
};

#endif

FunctionType::FunctionType(
        func_params params,
        std::unique_ptr<BaseType> returnType,
        bool isVariadic,
        bool isCapturing
) : params(std::move(params)), returnType(std::move(returnType)), isVariadic(isVariadic), isCapturing(isCapturing) {

};

bool FunctionType::satisfies(ValueType type) const {
    return type == ValueType::Lambda;
}

std::string FunctionType::representation() const {
    std::string rep("(");
    unsigned i = 0;
    auto size = params.size();
    while(i < size) {
        rep.append(params[i]->representation());
        if(i < size - 1) {
            rep.append(", ");
        }
        i++;
    }
    rep.append(") => ");
    rep.append(returnType->representation());
    return rep;
}

BaseType *FunctionType::copy() const {
    std::vector<std::unique_ptr<FunctionParam>> copied;
    for (auto &param: params) {
        copied.emplace_back(param->copy());
    }
    return new FunctionType(std::move(copied), std::unique_ptr<BaseType>(returnType->copy()), isVariadic, isCapturing);
}