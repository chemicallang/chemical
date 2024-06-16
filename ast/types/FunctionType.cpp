// Copyright (c) Qinetik 2024.

#include "FunctionType.h"
#include "ast/base/Value.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Type* llvm_func_return(Codegen &gen, BaseType* type) {
    if(type->value_type() == ValueType::Struct) {
        return gen.builder->getVoidTy();
    } else{
        return type->llvm_type(gen);
    }
}

std::vector<llvm::Type*> llvm_func_param_types(
        Codegen &gen,
        std::vector<std::unique_ptr<FunctionParam>>& params,
        BaseType* returnType,
        bool isCapturing,
        bool isVariadic
) {
    std::vector<llvm::Type*> paramTypes;
    // functions that return struct take a pointer to struct and actually return void
    // so allocation takes place outside function
    if(returnType->value_type() == ValueType::Struct) {
        paramTypes.emplace_back(gen.builder->getPtrTy());
    }
    // capturing lambdas gets a struct passed to them which contain captured data
    if(isCapturing) {
        paramTypes.emplace_back(gen.builder->getPtrTy());
    }
    auto size = isVariadic ? (params.size() - 1) : params.size();
    unsigned i = 0;
    while (i < size) {
        auto type = params[i]->type.get();
        paramTypes.emplace_back(type->llvm_param_type(gen));
        if(type->function_type() != nullptr) {
            auto func_type = type->function_type();
            // when a capturing lambda is a parameter, it is treated as two pointer parameters one for the lambda and another for it's data
            if(func_type->isCapturing) {
                paramTypes.emplace_back(gen.builder->getPtrTy());
            }
        }
        i++;
    }
    return paramTypes;
}

llvm::FunctionType *FunctionType::llvm_func_type(Codegen &gen) {
    return llvm::FunctionType::get(llvm_func_return(gen, returnType.get()), llvm_func_param_types(gen, params, returnType.get(), isCapturing, isVariadic), isVariadic);
}

llvm::Type *FunctionType::llvm_type(Codegen &gen) const {
    return gen.builder->getPtrTy();
};

llvm::Value *FunctionType::llvm_return_intercept(Codegen &gen, llvm::Value *value, ASTNode *node) {
    if(isCapturing) {
        if(node->as_func_param() != nullptr) {
            auto funcParam = node->as_func_param();
            return gen.pack_lambda((llvm::Function*) value, gen.current_function->getArg(funcParam->index + 1));
        } else {
            throw std::runtime_error("unknown calling node");
        }
    } else {
        return value;
    }
}

#endif

FunctionType::FunctionType(
        func_params params,
        std::unique_ptr<BaseType> returnType,
        bool isVariadic,
        bool isCapturing
) : BaseFunctionType(std::move(params), std::move(returnType), isVariadic), isCapturing(isCapturing) {

};

bool FunctionType::isInVarArgs(unsigned index) {
    return isVariadic && index >= (params.size() - 1);
}

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

void FunctionType::link(SymbolResolver &linker) {
    for (auto &param: params) {
        param->type->link(linker);
    }
    returnType->link(linker);
}

BaseType *FunctionType::copy() const {
    std::vector<std::unique_ptr<FunctionParam>> copied;
    for (auto &param: params) {
        copied.emplace_back(param->copy());
    }
    return new FunctionType(std::move(copied), std::unique_ptr<BaseType>(returnType->copy()), isVariadic, isCapturing);
}