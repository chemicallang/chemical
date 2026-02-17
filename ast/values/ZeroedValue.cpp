// Copyright (c) Chemical Language Foundation 2025.

#include "ZeroedValue.h"
#include "ast/structures/FunctionDeclaration.h"

#ifdef COMPILER_BUILD
#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Value* ZeroedValue::llvm_pointer(Codegen &gen) {
    const auto type = llvm_type(gen);
    const auto alloca = gen.builder->CreateAlloca(type);
    gen.builder->CreateStore(llvm::Constant::getNullValue(type), alloca);
    return alloca;
}

llvm::Value* ZeroedValue::llvm_value(Codegen &gen, BaseType *type) {
    return llvm::Constant::getNullValue(llvm_type(gen));
}

llvm::Value* ZeroedValue::llvm_ret_value(Codegen &gen, Value *returnValue) {
    const auto type = llvm_type(gen);
    if (type->isAggregateType()) {
        auto structPassed = gen.current_function->getArg(gen.current_func_type->getStructReturnArgIndex());
        gen.builder->CreateStore(llvm::Constant::getNullValue(type), structPassed);
        return nullptr;
    }
    return llvm::Constant::getNullValue(type);
}

llvm::AllocaInst* ZeroedValue::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) {
    const auto type = llvm_type(gen);
    const auto alloca = gen.builder->CreateAlloca(type, nullptr, identifier);
    gen.builder->CreateStore(llvm::Constant::getNullValue(type), alloca);
    return (llvm::AllocaInst*) alloca;
}

void ZeroedValue::llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) {
    const auto type = llvm_type(gen);
    gen.builder->CreateStore(llvm::Constant::getNullValue(type), lhsPtr);
}

unsigned int ZeroedValue::store_in_struct(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) {
    auto elementPtr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
    gen.builder->CreateStore(llvm::Constant::getNullValue(llvm_type(gen)), elementPtr);
    return index + 1;
}

unsigned int ZeroedValue::store_in_array(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) {
    auto elementPtr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
    gen.builder->CreateStore(llvm::Constant::getNullValue(llvm_type(gen)), elementPtr);
    return index + 1;
}

llvm::Type* ZeroedValue::llvm_type(Codegen& gen) {
    return getType()->llvm_type(gen);
}

#endif
