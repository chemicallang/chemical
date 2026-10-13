// Copyright (c) Chemical Language Foundation 2025.

#include "ArrayValue.h"
#include "StructValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
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
                } else if (linkedNode && linkedNode->kind() == ASTNodeKind::VariantDecl) {
                    auto variantDef = (VariantDefinition*) linkedNode;
                    if (values.empty()) {
                        values.reserve(numElements);
                        for (size_t i = 0; i < numElements; i++) {
                            auto elemType = getType()->elem_type.copy(scope.allocator);
                            auto structVal = new (scope.allocate<StructValue>()) StructValue(
                                (BaseType*) elemType,
                                (ExtendableMembersContainerNode*) variantDef,
                                (VariablesContainerBase*) nullptr,
                                encoded_location()
                            );
                            values.push_back(structVal);
                        }
                    }
                } else if (elemType->kind() == BaseTypeKind::Array) {
                    // Nested array element type: populate values with sub-ArrayValues
                    if (values.empty()) {
                        values.reserve(numElements);
                        auto innerArrType = elemType->as_array_type_unsafe();
                        for (size_t i = 0; i < numElements; i++) {
                            auto innerArrVal = new (scope.allocate<ArrayValue>()) ArrayValue(
                                encoded_location(),
                                (ArrayType*) getType()->elem_type->copy(scope.allocator)
                            );
                            innerArrVal->explicit_size = (unsigned int)innerArrType->get_array_size();
                            innerArrVal->evaluated_value(scope);
                            values.push_back(innerArrVal);
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
            } else if(elem->val_kind() != ValueKind::Identifier) {
                // Evaluate non-identifier, non-StructValue elements (DereferenceValue,
                // FunctionCall, etc.) so they produce their computed values.
                // Identifier elements (variable references) are NOT evaluated because
                // that would share the same struct with the source variable, causing
                // double-destruction. For those, move_clear_source handles cleanup.
                auto evalVal = elem->evaluated_value(scope);
                if(evalVal && evalVal != elem) {
                    values[i] = evalVal;
                }
            }
            // Implicit constructor conversion: if the element type is a struct with
            // an @implicit constructor matching this value, call it to produce the
            // correct struct value (e.g. [55] where element type ImpPair has an
            // @implicit constructor that takes int).
            auto& current = values[i];
            if(current && current->val_kind() != ValueKind::StructValue) {
                Value* resolved = current;
                if(current->val_kind() == ValueKind::Identifier) {
                    resolved = current->evaluated_value(scope);
                }
                if(resolved) {
                    auto elemTypeBase = known_elem_type();
                    if(elemTypeBase) {
                        auto imp_cons = elemTypeBase->implicit_constructor_for(resolved);
                        if(imp_cons) {
                            InterpretScope imp_scope(scope.global, scope.allocator, scope.global);
                            std::vector<Value*> imp_args = { resolved };
                            auto newVal = imp_cons->call(
                                &scope, imp_args, (Value*)nullptr, &imp_scope, true, (Value*)nullptr
                            );
                            if(newVal) {
                                values[i] = newVal;
                            }
                        }
                    }
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