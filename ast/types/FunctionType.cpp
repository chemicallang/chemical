// Copyright (c) Qinetik 2024.

#include "FunctionType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *FunctionType::llvm_type(Codegen &gen) const {
    std::vector<llvm::Type *> paramTypes;
    for (auto &param: params) {
        paramTypes.push_back(param->llvm_type(gen));
    }
    return llvm::FunctionType::get(returnType->llvm_type(gen), paramTypes, isVariadic);
};

llvm::Type *FunctionType::llvm_param_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

#endif

FunctionType::FunctionType(
        func_params params,
        std::unique_ptr<BaseType> returnType,
        bool isVariadic
) : params(std::move(params)), returnType(std::move(returnType)), isVariadic(isVariadic) {

};

bool FunctionType::satisfies(ValueType type) const {
    return type == ValueType::Lambda;
}

std::string FunctionType::representation() const {
    return "() => " + returnType->representation();
}

BaseType *FunctionType::copy() const {
    std::vector<std::unique_ptr<FunctionParam>> copied;
    for (auto &param: params) {
        copied.emplace_back(param->copy());
    }
    return new FunctionType(std::move(copied), std::unique_ptr<BaseType>(returnType->copy()), isVariadic);
}