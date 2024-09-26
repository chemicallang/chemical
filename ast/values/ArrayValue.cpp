// Copyright (c) Qinetik 2024.

#include "ArrayValue.h"
#include "StructValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/ArrayType.h"
#include "ast/utils/ASTUtils.h"
#include "compiler/SymbolResolver.h"
#include "ast/values/FunctionCall.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

BaseType* array_child(BaseType* expected_type) {
    if(expected_type) {
        auto pure_type = expected_type->pure_type();
        if(pure_type->kind() == BaseTypeKind::Array) {
            return pure_type->known_child_type();
        }
    }
    return nullptr;
}

llvm::Value *ArrayValue::llvm_pointer(Codegen &gen) {
    return arr;
}

BaseType* array_child_type(ArrayValue& value, ASTAllocator& allocator) {
    return ((ArrayType*) value.create_type(allocator))->elem_type;
}

void ArrayValue::initialize_allocated(Codegen& gen, llvm::Value* allocated, BaseType* expected_type) {
    // filling array with values
    std::vector<llvm::Value*> idxList;
    idxList.emplace_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    auto child_type = array_child(expected_type);
    auto known_child_t = element_type(gen.allocator);
    const auto def = known_child_t->linked_struct_def();
    auto parent_type = llvm_type(gen);
    bool moved = false;
    for (size_t i = 0; i < values.size(); ++i) {

        // replace values that are calls to implicit constructor
        if(def) {
            const auto implicit = def->implicit_constructor_func(gen.allocator, values[i]);
            if(implicit) {
                values[i] = call_with_arg(implicit, values[i], gen.allocator);
            }
        }

        auto& value = *values[i];
        moved = false;
        if(value.is_ref_moved()) {
            // moving the struct
            std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
            auto elementPtr = Value::get_element_pointer(gen, parent_type, allocated, idx, i);
            moved = gen.move_by_memcpy(known_child_t, &value, elementPtr, value.llvm_value(gen, nullptr));
        }
        if(!moved) {
            if(gen.requires_memcpy_ref_struct(known_child_t, &value)) {
                std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
                auto elementPtr = Value::get_element_pointer(gen, parent_type, allocated, idx, i);
                gen.memcpy_struct(value.llvm_type(gen), elementPtr, value.llvm_value(gen, nullptr));
            } else {
                // couldn't move the struct
                value.store_in_array(gen, this, allocated, parent_type, idxList, i, child_type);
            }
        }
    }
}

llvm::AllocaInst* ArrayValue::llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) {
    auto arrType = llvm_type(gen);
    arr = gen.builder->CreateAlloca(arrType, nullptr, identifier);
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

llvm::Value *ArrayValue::llvm_arg_value(Codegen &gen, FunctionCall *call, unsigned int index) {
    return llvm_allocate(gen, "", call->get_arg_type(index));
}

void ArrayValue::llvm_destruct(Codegen &gen, llvm::Value *allocaInst) {
    auto elem_type = element_type(gen.allocator);
    gen.destruct(allocaInst, array_size(), elem_type, [](llvm::Value*){});
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
    if (elemType) {
        if(sizes.size() <= 1) {
            // get empty array type from the user
            elementType = elemType->llvm_type(gen);
        } else {
            unsigned int i = sizes.size() - 1;
            while(i > 0) {
                if(i == sizes.size() - 1) {
                    elementType = llvm::ArrayType::get(elemType->llvm_type(gen), sizes[i]);
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
        return elemType->linked_node();
    } else {
        return values[0]->linked_node();
    }
}

bool ArrayValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    if(elemType) {
        elemType->link(linker);
        const auto elem_type = element_type(linker.allocator);
        const auto def = elem_type->linked_struct_def();
        if(def) {
            unsigned i = 0;
            while (i < values.size()) {
                values[i]->link(linker, values[i], elemType);
                const auto implicit = def->implicit_constructor_func(linker.allocator, values[i]);
                if(implicit) {
                    link_with_implicit_constructor(implicit, linker, values[i]);
                }
                i++;
            }
            created_type = create_type(linker.allocator);
            return true;
        }
    } else if(expected_type && expected_type->kind() == BaseTypeKind::Array) {
        const auto arr_type = (ArrayType*) expected_type;
        elemType = arr_type->elem_type;
    }
    auto& current_func_type = *linker.current_func_type;
    BaseType* known_elem_type = nullptr;
    if(elemType) {
        known_elem_type = elemType;
    }
    unsigned i = 0;
    for(auto& value : values) {
        if(value->link(linker, value, nullptr) && i == 0 && !known_elem_type) {
            known_elem_type = value->known_type();
        }
        if(known_elem_type) {
            current_func_type.mark_moved_value(linker.allocator, value, known_elem_type, linker, elemType != nullptr);
        }
        i++;
    }
    created_type = create_type(linker.allocator);
    return true;
}

BaseType* ArrayValue::element_type(ASTAllocator& allocator) const {
    BaseType *elementType;
    if (elemType) {
        if(sizes.size() <= 1) {
            // get empty array type from the user
            elementType = elemType;
        } else {
            unsigned int i = sizes.size() - 1;
            while(i > 0) {
                if(i == sizes.size() - 1) {
                    elementType = new (allocator.allocate<ArrayType>()) ArrayType(elemType, sizes[i], token);
                } else {
                    elementType = new (allocator.allocate<ArrayType>()) ArrayType(elementType, sizes[i], token);
                }
                i--;
            }
        }
    } else {
        if(values.empty()) {
            elementType = nullptr;
        } else {
            elementType = values[0]->create_type(allocator);
        }
    }
    return elementType;
}

BaseType* ArrayValue::create_type(ASTAllocator& allocator) {
    return new (allocator.allocate<ArrayType>()) ArrayType(element_type(allocator), array_size(), nullptr);
}

//hybrid_ptr<BaseType> ArrayValue::get_base_type() {
//    if(!cached_type) {
//        cached_type = std::make_unique<ArrayType>(element_type(), array_size(), nullptr);
//    }
//    return hybrid_ptr<BaseType> { cached_type.get(), false };
//}

BaseType* ArrayValue::known_type() {
    return created_type;
}