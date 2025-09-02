// Copyright (c) Chemical Language Foundation 2025.

#include "ArrayValue.h"
#include "StructValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/ArrayType.h"
#include "ast/utils/ASTUtils.h"
#include "ast/values/FunctionCall.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

BaseType* array_child(BaseType* expected_type) {
    if(expected_type) {
        auto pure_type = expected_type->canonical();
        if(pure_type->kind() == BaseTypeKind::Array) {
            return pure_type->known_child_type();
        }
    }
    return nullptr;
}

llvm::Value *ArrayValue::llvm_pointer(Codegen &gen) {
    return arr;
}

void ArrayValue::initialize_allocated(Codegen& gen, llvm::Value* allocated, BaseType* expected_type) {
    // filling array with values
    std::vector<llvm::Value*> idxList;
    idxList.emplace_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    auto child_type = array_child(expected_type);
    auto known_child_t = element_type(gen.allocator)->canonical();
    const auto def = known_child_t->linked_struct_def();
    auto parent_type = llvm_type(gen);
    bool moved = false;
    for (size_t i = 0; i < values.size(); ++i) {

        const auto implicit = def ? def->implicit_constructor_func(gen.allocator, values[i]) : nullptr;
        if(implicit) {
            // replace values that are calls to implicit constructor
            values[i] = (Value*) call_with_arg(implicit, values[i], known_child_t, gen.allocator, gen);
        } else {
            if(known_child_t->kind() == BaseTypeKind::Reference) {
                // store pointer only, since user want's a reference
                std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
                auto elementPtr = Value::get_element_pointer(gen, parent_type, allocated, idx, i);
                const auto ref_pointer = values[i]->llvm_pointer(gen);
                const auto storeInst = gen.builder->CreateStore(ref_pointer, elementPtr);
                gen.di.instr(storeInst, values[i]);
                continue;
            }
        }

        auto& value = *values[i];
//        moved = false;
//        if(value.is_ref_moved()) {
//            // moving the struct
//            std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
//            auto elementPtr = Value::get_element_pointer(gen, parent_type, allocated, idx, i);
//            moved = gen.move_by_memcpy(known_child_t, &value, elementPtr, value.llvm_value(gen, nullptr));
//        }
//        if(!moved) {
//            if(gen.requires_memcpy_ref_struct(known_child_t, &value)) {
//                std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
//                auto elementPtr = Value::get_element_pointer(gen, parent_type, allocated, idx, i);
//                gen.memcpy_struct(value.llvm_type(gen), elementPtr, value.llvm_value(gen, nullptr), value.encoded_location());
//            } else {
                // couldn't move the struct
                value.store_in_array(gen, this, allocated, parent_type, idxList, i, known_child_t);
//            }
//        }
    }
}

llvm::AllocaInst* ArrayValue::llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) {
    auto arrType = llvm_type(gen);
    const auto allocaInst = gen.builder->CreateAlloca(arrType, nullptr, identifier);
    gen.di.instr(allocaInst, this);
    arr = allocaInst;
    initialize_allocated(gen, arr, expected_type);
    return arr;
}

llvm::Value *ArrayValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    // Why is this here :
    // Well when user declares a multidimensional array, we allocate memory for the whole thing
    // then we call llvm_value on values, if we implement it, this would allocate memory for nested array as well
    // resulting in allocating memory two times for an array
    throw std::runtime_error("memory for array value wasn't allocated");
}

llvm::Value *ArrayValue::llvm_arg_value(Codegen &gen, BaseType* expected_type) {
    return llvm_allocate(gen, "", expected_type);
}

unsigned int ArrayValue::store_in_array(
        Codegen &gen,
        Value *parent,
        llvm::Value* allocated,
        llvm::Type* allocated_type,
        std::vector<llvm::Value *> idxList,
        unsigned int index,
        BaseType* expected_type
) {
    auto elem_pointer = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
    initialize_allocated(gen, elem_pointer, expected_type);
    return index + values.size();
}

unsigned int ArrayValue::store_in_struct(
        Codegen &gen,
        Value *parent,
        llvm::Value *allocated,
        llvm::Type* allocated_type,
        std::vector<llvm::Value *> idxList,
        unsigned int index,
        BaseType* expected_type
) {
    auto elem_pointer = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
    initialize_allocated(gen, elem_pointer, expected_type);
    return index + values.size();
}

llvm::Type *ArrayValue::llvm_elem_type(Codegen &gen) {
    llvm::Type *elementType;
    const auto elemType = known_elem_type();
    if (elemType) {
        // get empty array type from the user
        elementType = elemType->llvm_type(gen);
    } else {
        elementType = values[0]->llvm_type(gen);
    }
    return elementType;
}

llvm::Type *ArrayValue::llvm_type(Codegen &gen) {
    return llvm::ArrayType::get(llvm_elem_type(gen), array_size());
}

bool ArrayValue::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    return linked_node()->add_child_index(gen, indexes, name);
}

#endif

TypeLoc& ArrayValue::known_elem_type() const {
    return getType()->elem_type;
}

ASTNode *ArrayValue::linked_node() {
    if(values.empty()) {
        return known_elem_type()->linked_node();
    } else {
        return values[0]->linked_node();
    }
}

//void ArrayValue::relink_after_generic(SymbolResolver &linker, BaseType *expected_type) {
//    if(!created_type->elem_type && expected_type && expected_type->kind() == BaseTypeKind::Array) {
//        const auto known_elem_type = ((ArrayType*) expected_type)->elem_type->pure_type(linker.allocator);
//        if(known_elem_type) {
//            created_type->elem_type = known_elem_type->copy(*linker.ast_allocator);
//        }
//    }
//}

BaseType* ArrayValue::element_type(ASTAllocator& allocator) const {
    TypeLoc elementType(nullptr);
    const auto known = known_elem_type();
    if (known) {
        // get empty array type from the user
        elementType = known;
    } else {
        if(values.empty()) {
            elementType = nullptr;
        } else {
            elementType = {values[0]->getType(), encoded_location()};
        }
    }
    return elementType;
}

BaseType* ArrayValue::known_type() {
    return getType();
}

void ArrayValue::determine_type(ASTAllocator& allocator) {
    getType()->elem_type = { element_type(allocator), encoded_location() };
}