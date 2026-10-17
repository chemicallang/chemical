// Copyright (c) Chemical Language Foundation 2025.

#include "IncDecValue.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/StructValue.h"
#include "ast/values/IntNumValue.h"
#include "compiler/ASTDiagnoser.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/PointerType.h"
#include "ast/values/NullValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/PointerValue.h"
#include "compiler/symres/ImplementationsIndex.h"
#include "compiler/lab/LabBuildCompiler.h"

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

BaseType* IncDecValue::determine_type(ASTDiagnoser& diagnoser, const CoreNodes& coreNodes, const ImplementationsIndex& implsIndex) {
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

uint64_t NullValue::byte_size(const TargetData& data) {
    return data.is64Bit ? 8 : 4;
}