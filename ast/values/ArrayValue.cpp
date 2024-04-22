// Copyright (c) Qinetik 2024.

#include "ArrayValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "ast/types/ArrayType.h"

llvm::Value *ArrayValue::llvm_pointer(Codegen &gen) {
    return arr;
}

llvm::AllocaInst* ArrayValue::llvm_allocate(Codegen &gen, const std::string &identifier) {
    auto arrType = llvm_type(gen);
    arr = gen.builder->CreateAlloca(arrType, nullptr, identifier);
    // filling array with values
    for (size_t i = 0; i < values.size(); ++i) {
        auto index = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), i);
        auto elemPtr = gen.builder->CreateGEP(arrType, arr,
                                              {llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0), index});
        auto elemValue = values[i]->llvm_value(gen);
        gen.builder->CreateStore(elemValue, elemPtr);
    }
    return arr;
}

llvm::Value *ArrayValue::llvm_value(Codegen &gen) {
    throw std::runtime_error("cannot allocate an array without an identifier");
}

llvm::Type *ArrayValue::llvm_elem_type(Codegen &gen) {
    llvm::Type *elementType;
    if (values.empty()) {
        // get empty array type from the user
        elementType = elemType.value()->llvm_type(gen);
    } else {
        elementType = values[0]->llvm_type(gen);
    }
    return elementType;
}

llvm::Type *ArrayValue::llvm_type(Codegen &gen) {
    return llvm::ArrayType::get(llvm_elem_type(gen), array_size());
}

bool ArrayValue::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, unsigned int index) {
    indexes.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    indexes.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), index));
    return true;
}

#endif

std::unique_ptr<BaseType> ArrayValue::create_type() const {
    BaseType* arrElemType;
    if(elemType.has_value()) {
        arrElemType = elemType.value()->copy();
    } else {
        if(!values.empty()) {
            arrElemType = values[0]->create_type().release();
        } else {
            // TODO report error
            return nullptr;
        }
    }
    return std::make_unique<ArrayType>(std::unique_ptr<BaseType>(arrElemType), array_size());
}