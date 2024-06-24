// Copyright (c) Qinetik 2024.

#include "ArrayValue.h"
#include "StructValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/ArrayType.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

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
         values[i]->store_in_array(gen, this, arr, idxList, i);
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

void ArrayValue::llvm_destruct(Codegen &gen, llvm::Value *allocaInst) {
    auto elem_type = element_type();
    if(!elem_type->linked_node() || !elem_type->linked_node()->as_struct_def()) {
        return;
    }
    auto structDef = elem_type->linked_node()->as_struct_def();
    auto destructorFunc = structDef->destructor_func();
    if(!destructorFunc) {
        return;
    }
    auto firstEle = gen.builder->CreateGEP(llvm_type(gen), allocaInst, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
    auto lastEle = gen.builder->CreateGEP(llvm_elem_type(gen), firstEle, { gen.builder->getInt32(array_size()) }, "", gen.inbounds);
    auto current_block = gen.builder->GetInsertBlock();
    auto body_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
    gen.CreateBr(body_block);
    gen.SetInsertPoint(body_block);
    auto PHI = gen.builder->CreatePHI(gen.builder->getPtrTy(), 2);
    PHI->addIncoming(lastEle, current_block);
    auto structPtr = gen.builder->CreateGEP(llvm_elem_type(gen), PHI, { gen.builder->getInt32(-1) }, "", gen.inbounds);
    PHI->addIncoming(structPtr, body_block);
    std::vector<llvm::Value*> args;
    if(destructorFunc->has_self_param()) {
        args.emplace_back(structPtr);
    }
    gen.builder->CreateCall(destructorFunc->llvm_func_type(gen), destructorFunc->funcCallee, args, "");
    auto result = gen.builder->CreateICmpEQ(structPtr, firstEle);
    auto end_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
    gen.CreateCondBr(result, end_block, body_block);
    gen.SetInsertPoint(end_block);
}

unsigned int ArrayValue::store_in_array(
        Codegen &gen,
        ArrayValue *parent,
        llvm::AllocaInst* ptr,
        std::vector<llvm::Value *> idxList,
        unsigned int index
) {
    idxList.emplace_back(gen.builder->getInt32(index));
    for (size_t i = 0; i < values.size(); ++i) {
        values[i]->store_in_array(gen, parent, ptr, idxList, i);
    }
    return index + values.size();
}

unsigned int ArrayValue::store_in_struct(
        Codegen &gen,
        StructValue *parent,
        llvm::Value *allocated,
        std::vector<llvm::Value *> idxList,
        unsigned int index
) {
    idxList.emplace_back(gen.builder->getInt32(index));
    for (size_t i = 0; i < values.size(); ++i) {
        values[i]->store_in_struct(gen, parent, allocated, idxList, i);
    }
    return index + 1;
}

llvm::Type *ArrayValue::llvm_elem_type(Codegen &gen) {
    llvm::Type *elementType;
    if (values.empty()) {
        if(sizes.size() <= 1) {
            // get empty array type from the user
            elementType = elemType.value()->llvm_type(gen);
        } else {
            unsigned int i = sizes.size() - 1;
            while(i > 0) {
                if(i == sizes.size() - 1) {
                    elementType = llvm::ArrayType::get(elemType.value()->llvm_type(gen), sizes[i]);
                } else {
                    elementType = llvm::ArrayType::get(elementType, sizes[i]);
                }
                i--;
            }
        }
    } else {
        elementType = values[0]->llvm_type(gen);
    }
    return elementType;
}

llvm::Type *ArrayValue::llvm_type(Codegen &gen) {
    return llvm::ArrayType::get(llvm_elem_type(gen), array_size());
}

bool ArrayValue::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return linked_node()->add_child_index(gen, indexes, name);
}

#endif

ASTNode *ArrayValue::linked_node() {
    if(values.empty()) {
        return elemType.value()->linked_node();
    } else {
        return values[0]->linked_node();
    }
}

void ArrayValue::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) {
    for(auto& value : values) {
        value->link(linker, value);
    }
    if(elemType.has_value()) {
        elemType.value()->link(linker);
    }
}

std::unique_ptr<BaseType> ArrayValue::element_type() const {
    BaseType *elementType;
    if (values.empty()) {
        if(sizes.size() <= 1) {
            // get empty array type from the user
            elementType = elemType.value()->copy();
        } else {
            unsigned int i = sizes.size() - 1;
            while(i > 0) {
                if(i == sizes.size() - 1) {
                    elementType = new ArrayType(std::unique_ptr<BaseType>(elemType.value()->copy()), sizes[i]);
                } else {
                    elementType = new ArrayType(std::unique_ptr<BaseType>(elementType), sizes[i]);
                }
                i--;
            }
        }
    } else {
        elementType = values[0]->create_type().release();
    }
    return std::unique_ptr<BaseType>(elementType);
}

std::unique_ptr<BaseType> ArrayValue::create_type() const {
    return std::make_unique<ArrayType>(std::move(element_type()), array_size());
}