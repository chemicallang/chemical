// Copyright (c) Qinetik 2024.

#include "ArrayValue.h"
#include "StructValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/ArrayType.h"
#include "ast/utils/ASTUtils.h"

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

llvm::Value *ArrayValue::llvm_arg_value(Codegen &gen, FunctionCall *call, unsigned int index) {
    return llvm_allocate(gen, "");
}

void ArrayValue::llvm_destruct(Codegen &gen, llvm::Value *allocaInst) {
    auto elem_type = element_type();
    gen.destruct(allocaInst, array_size(), elem_type.get());
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
    if(elemType.has_value()) {
        elemType.value()->link(linker, elemType.value());
        const auto elem_type = element_type();
        const auto def = elem_type->linked_struct_def();
        if(def) {
            unsigned i = 0;
            while (i < values.size()) {
                values[i]->link(linker, values[i]);
                const auto implicit = def->implicit_constructor_func(values[i].get());
                if(implicit) {
                    values[i] = call_with_arg(implicit, std::move(values[i]), linker);
                }
                i++;
            }
            return;
        }
    }
    for(auto& value : values) {
        value->link(linker, value);
    }
}

std::unique_ptr<BaseType> ArrayValue::element_type() const {
    BaseType *elementType;
    if (elemType.has_value()) {
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

std::unique_ptr<BaseType> ArrayValue::create_type() {
    return std::make_unique<ArrayType>(std::move(element_type()), array_size());
}

hybrid_ptr<BaseType> ArrayValue::get_base_type() {
    return hybrid_ptr<BaseType> { new ArrayType(std::move(element_type()), array_size()) };
}