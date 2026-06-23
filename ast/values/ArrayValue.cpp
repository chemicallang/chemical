// Copyright (c) Chemical Language Foundation 2025.

#include "ArrayValue.h"
#include "StructValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/ArrayType.h"
#include "ast/utils/ASTUtils.h"
#include "ast/values/FunctionCall.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include <cstring>

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
    const auto def = known_child_t->get_direct_linked_struct();
    auto parent_type = llvm_type(gen);
    bool moved = false;
    for (size_t i = 0; i < values.size(); ++i) {

        const auto implicit = def ? def->implicit_constructor_func(values[i]) : nullptr;
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
    std::vector<llvm::Constant*> llvm_vals;
    auto known_child_t = element_type(gen.allocator)->canonical();
    for(const auto val : values) {
        const auto final_val = val->llvm_value(gen, known_child_t);
        if(llvm::isa<llvm::Constant>(final_val)) {
            llvm_vals.emplace_back((llvm::Constant*) final_val);
        } else {
            gen.error("expected value to result in a constant expression", val);
        }
    }
    return llvm::ConstantArray::get((llvm::ArrayType*) llvm_type(gen), llvm_vals);
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
    if (expected_type && expected_type->isPointerCanonical()) {
        auto alloc = llvm_allocate(gen, "", expected_type);
        gen.llvm.CreateStore(alloc, elem_pointer, encoded_location());
    } else {
        initialize_allocated(gen, elem_pointer, expected_type);
    }
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

Value* ArrayValue::evaluated_value(InterpretScope& scope) {
    // Lazily allocate contiguous memory for primitive element types, or populate
    // StructValue elements for struct element types.
    if (!contiguousData) {
        const auto elemType = known_elem_type();
        if (elemType) {
            const auto byteSize = elemType->byte_size(scope.global->target_data);
            // Compute number of elements
            size_t numElements = 0;
            if (!values.empty()) {
                numElements = values.size();
            } else if (explicit_size > 0) {
                numElements = explicit_size;
            } else {
                const auto arrType = getType();
                if (arrType) {
                    numElements = arrType->get_array_size();
                }
            }
            if (numElements > 0) {
                const auto linkedNode = elemType->get_direct_linked_canonical_node();
                // Struct element types (byteSize may be > 8 or 0): populate values with StructValues
                if (linkedNode && linkedNode->kind() == ASTNodeKind::StructDecl) {
                    auto structDef = (StructDefinition*) linkedNode;
                    // Only populate if values hasn't been initialized yet
                    if (values.empty()) {
                        values.reserve(numElements);
                        for (size_t i = 0; i < numElements; i++) {
                            auto structVal = new (scope.allocate<StructValue>()) StructValue(
                                getType()->elem_type.copy(scope.allocator),
                                structDef,
                                (VariablesContainerBase*) structDef,
                                encoded_location()
                            );
                            for (const auto field : structDef->variables()) {
                                auto defValue = field->default_value();
                                if (defValue) {
                                    structVal->values.emplace(
                                        field->name,
                                        StructMemberInitializer{field->name, defValue->scope_value(scope)}
                                    );
                                } else {
                                    structVal->values.emplace(
                                        field->name,
                                        StructMemberInitializer{field->name, scope.getNullValue()}
                                    );
                                }
                            }
                            values.push_back(structVal);
                        }
                    }
                } else if (byteSize > 0 && byteSize <= 8) {
                    // Primitive types: allocate contiguousData
                    const auto totalSize = numElements * byteSize;
                    auto* mem = (char*)scope.allocator.allocate_released_size(totalSize, alignof(uint64_t));
                    if (mem) {
                        contiguousData = mem;
                        contiguousSize = totalSize;
                        std::memset(mem, 0, totalSize);
                        // Copy initialized values into contiguous memory
                        for (size_t i = 0; i < values.size(); i++) {
                            auto evaluatedVal = values[i]->evaluated_value(scope);
                            const auto num = evaluatedVal ? evaluatedVal->get_number() : std::nullopt;
                            if (num.has_value()) {
                                std::memcpy(mem + i * byteSize, &num.value(), byteSize);
                            }
                        }
                    }
                }
            }
        }
    }
    // For arrays that already have struct elements (from initializers like [Struct{...}]),
    // evaluate each element to get runtime values with properly evaluated field initializers.
    // Only runs ONCE — without this guard, AddrOfValue::evaluated_value() calls
    // arrVal->evaluated_value() on every &arr[i] access, which replaces ALL array
    // elements with fresh default copies, wiping out modifications made through pointers.
    if(!structElementsEvaluated && !values.empty()) {
        structElementsEvaluated = true;
        for(size_t i = 0; i < values.size(); i++) {
            auto elem = values[i];
            if(elem->val_kind() == ValueKind::StructValue) {
                auto sv = (StructValue*)elem;
                auto sv_eval = sv->evaluated_value(scope);
                if(sv_eval && sv_eval != elem) {
                    values[i] = sv_eval;
                }
            }
        }
    }
    return this;
}

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

void ArrayValue::determine_type(ASTAllocator& allocator) {
    getType()->elem_type = { element_type(allocator), encoded_location() };
}