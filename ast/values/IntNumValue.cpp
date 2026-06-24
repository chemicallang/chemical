// Copyright (c) Chemical Language Foundation 2025.

#include "IncDecValue.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/MembersContainer.h"
#include "ast/values/IntNumValue.h"
#include "compiler/ASTDiagnoser.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/PointerType.h"
#include "ast/values/NullValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/PointerValue.h"
#include "compiler/symres/ImplementationsIndex.h"

IntNumValue* IntNumValue::create_number(
        ASTAllocator& alloc,
        TypeBuilder& typeBuilder,
        unsigned int bitWidth,
        bool is_signed,
        uint64_t value,
        SourceLocation location
) {
    return new (alloc.allocate<IntNumValue>()) IntNumValue(value, typeBuilder.getIntNType(bitWidth, !is_signed), location);
}

Value* IncDecValue::evaluated_value(InterpretScope &scope) {
    // Check if the inner value has a reference type (&mut int, &int).
    // In the interpreter, references are stored as PointerValues pointing to the actual
    // data. We need to increment the pointed-to VALUE, not the pointer itself.
    // Regular pointers (*mut int, *int) should use the normal pointer arithmetic path.
    const auto innerType = value->getType();
    if(innerType && innerType->canonical()->kind() == BaseTypeKind::Reference) {
        const auto rawVal = value->evaluated_value(scope);
        if(rawVal && rawVal->val_kind() == ValueKind::PointerValue) {
            auto ptrVal = (PointerValue*)rawVal;
            auto dereffed = ptrVal->deref(scope, encoded_location(), this);
            if(dereffed) {
                auto num = dereffed->get_number();
                if(num.has_value()) {
                    auto oldNum = num.value();
                    uint64_t newNum = increment ? oldNum + 1 : oldNum - 1;
                    const auto byteSize = ptrVal->getType()->byte_size(scope.global->target_data);
                    if(byteSize > 0 && byteSize <= 8) {
                        std::memcpy(ptrVal->data, &newNum, byteSize);
                    }
                    if(post) {
                        return new (scope.allocate<IntNumValue>()) IntNumValue(
                            oldNum,
                            scope.global->typeBuilder.getIntNType((unsigned int)(byteSize * 8), true),
                            encoded_location()
                        );
                    } else {
                        return new (scope.allocate<IntNumValue>()) IntNumValue(
                            newNum,
                            scope.global->typeBuilder.getIntNType((unsigned int)(byteSize * 8), true),
                            encoded_location()
                        );
                    }
                }
            }
        }
    }
    
    // Apply the increment/decrement operation (normal path for integers and pointers)
    const auto oldVal = value->evaluated_value(scope);
    const auto delta = new (scope.allocate<IntNumValue>()) IntNumValue(1, scope.global->typeBuilder.getShortType(), encoded_location());
    value->set_value(scope, delta, increment ? Operation::Addition : Operation::Subtraction, encoded_location());
    
    // For post-increment/decrement (i++), return the old value
    // For pre-increment/decrement (++i), return the new value
    if(post) {
        return oldVal;
    } else {
        return value->evaluated_value(scope);
    }
}

BaseType* IncDecValue::determine_type(ASTDiagnoser& diagnoser, CoreNodes& coreNodes, ImplementationsIndex& implsIndex) {
    const auto type = value->getType();
    // checking if operator is overloaded
    const auto node = type->get_linked_canonical_node(true, false);
    if(node) {
        const auto container = node->get_members_container();
        if(container) {
            const auto func = implsIndex.get_inc_dec_op_impl(coreNodes, container, increment, post);
            if (func == nullptr) {
                diagnoser.error(this) << "operator implementation not found";
                return (BaseType*) type;
            }
            if(func->params.size() != 1) {
                // since this expression has two values, we always expect two parameters
                diagnoser.error(this) << "expected operator implementation function to have exactly one parameter";
                return (BaseType*) type;
            }
            return func->returnType;
        }
    }
    // normal flow
    const auto pure = type->canonical();
    if(pure->kind() == BaseTypeKind::Reference) {
        const auto ref = pure->as_reference_type_unsafe();
        const auto ref_type = ref->type->canonical();
        if(BaseType::isLoadableReferencee(ref_type->kind())) {
            return ref_type;
        } else {
            return ref;
        }
    } else {
        return type;
    }
}

uint64_t NullValue::byte_size(TargetData& data) {
    return data.is64Bit ? 8 : 4;
}