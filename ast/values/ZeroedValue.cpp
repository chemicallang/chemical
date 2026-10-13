// Copyright (c) Chemical Language Foundation 2025.

#include "ZeroedValue.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/values/StructValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/ArrayType.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/NullValue.h"
#include "ast/types/IntNType.h"
#include "ast/types/FloatType.h"
#include "ast/types/DoubleType.h"

Value* ZeroedValue::evaluated_value(InterpretScope& scope) {
    // For struct types, create a properly zero-initialized StructValue so
    // that member access like z.a works via child() on the resulting StructValue.
    auto type = getType();
    if(type) {
        auto linkedNode = type->get_direct_linked_canonical_node();
        if(linkedNode && linkedNode->kind() == ASTNodeKind::StructDecl) {
            auto structDef = (StructDefinition*) linkedNode;
            auto container = (VariablesContainerBase*) structDef;
            auto structVal = new (scope.allocate<StructValue>()) StructValue(
                TypeLoc(getType()->copy(scope.allocator), encoded_location()),
                structDef,
                container,
                encoded_location()
            );
            for(const auto field : structDef->variables()) {
                auto defValue = field->default_value();
                if(defValue) {
                    structVal->values.emplace(
                        field->name,
                        StructMemberInitializer{field->name, defValue->scope_value(scope)}
                    );
                } else {
                    // Create proper zero value based on field type
                    auto fieldType = field->known_type();
                    if(!fieldType) {
                        structVal->values.emplace(
                            field->name,
                            StructMemberInitializer{field->name, scope.getNullValue()}
                        );
                    } else {
                        auto pureType = fieldType->canonical();
                        auto kind = pureType->kind();
                        Value* zeroVal = nullptr;
                        if(kind == BaseTypeKind::IntN) {
                            zeroVal = new (scope.allocate<IntNumValue>()) IntNumValue(
                                0, (IntNType*)pureType, encoded_location()
                            );
                        } else if(kind == BaseTypeKind::Bool) {
                            zeroVal = new (scope.allocate<BoolValue>()) BoolValue(
                                false, scope.global->typeBuilder.getBoolType(), encoded_location()
                            );
                        } else if(kind == BaseTypeKind::Float) {
                            zeroVal = new (scope.allocate<FloatValue>()) FloatValue(
                                0.0f, (FloatType*)pureType, encoded_location()
                            );
                        } else if(kind == BaseTypeKind::Double) {
                            zeroVal = new (scope.allocate<DoubleValue>()) DoubleValue(
                                0.0, (DoubleType*)pureType, encoded_location()
                            );
                        } else if(kind == BaseTypeKind::Pointer) {
                            zeroVal = new (scope.allocate<NullValue>()) NullValue(
                                scope.global->typeBuilder.getNullPtrType(), encoded_location()
                            );
                        } else {
                            zeroVal = scope.getNullValue();
                        }
                        if(zeroVal) {
                            structVal->values.emplace(
                                field->name,
                                StructMemberInitializer{field->name, zeroVal}
                            );
                        }
                    }
                }
            }
            return structVal;
        }
        // For array types, materialize an ArrayValue so that indexing and
        // element assignment work (`IndexOperator` only understands ArrayValue /
        // PointerValue / StructValue). ArrayValue::evaluated_value allocates
        // zero-filled contiguous storage for primitive element types and default
        // elements for struct/variant/nested-array element types.
        auto canonical = type->canonical();
        if(canonical && canonical->kind() == BaseTypeKind::Array) {
            auto arrType = canonical->as_array_type_unsafe();
            auto size = arrType->get_array_size();
            if(size > 0) {
                auto arrVal = new (scope.allocate<ArrayValue>()) ArrayValue(
                    encoded_location(),
                    (ArrayType*) arrType->copy(scope.allocator)
                );
                arrVal->explicit_size = (unsigned int) size;
                arrVal->evaluated_value(scope);
                return arrVal;
            }
        }
    }
    // For primitive types, return the ZeroedValue itself
    return this;
}

Value* ZeroedValue::child(InterpretScope& scope, const chem::string_view& name) {
    // First evaluate to get the proper struct value, then access child
    auto eval = evaluated_value(scope);
    if(eval && eval != this) {
        return eval->child(scope, name);
    }
    return nullptr;
}

