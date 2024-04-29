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
    std::vector<llvm::Value*> idxList;
    idxList.emplace_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    for (size_t i = 0; i < values.size(); ++i) {
         values[i]->store_in_array(gen, this, idxList, i);
    }
    return arr;
}

llvm::Value *ArrayValue::llvm_value(Codegen &gen) {
    // Why is this here :
    // Well when user declares a multidimensional array, we allocate memory for the whole thing
    // then we call llvm_value on values, if we implement it, this would allocate memory for nested array as well
    // resulting in allocating memory two times for an array
    throw std::runtime_error("cannot allocate an array without an identifier");
}

unsigned int ArrayValue::store_in_array(
        Codegen &gen,
        ArrayValue *parent,
        std::vector<llvm::Value *> idxList,
        unsigned int index
) {
    idxList.emplace_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), index));
    for (size_t i = 0; i < values.size(); ++i) {
        values[i]->store_in_array(gen, parent, idxList, i);
    }
    return index + values.size();
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

bool ArrayValue::add_child_indexes(Codegen &gen, std::vector<llvm::Value *> &indexes, std::vector<std::unique_ptr<Value>> &u_inds) {
    indexes.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    for(auto& value : u_inds) {
        indexes.emplace_back(value->llvm_value(gen));
    }
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