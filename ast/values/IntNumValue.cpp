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
    const auto val = new (scope.allocate<IntNumValue>()) IntNumValue(1, scope.global->typeBuilder.getShortType(), encoded_location());
    value->set_value(scope, val, increment ? Operation::Addition : Operation::Subtraction, encoded_location());
    // TODO support post and pre properly
    return value->evaluated_value(scope);
}

chem::string_view IncDecValue::get_overloaded_func_name() {
    return increment ? (
        post ? "inc_post" : "inc_pre"
    ) : (
        post ? "dec_post" : "dec_pre"
    );
}

BaseType* IncDecValue::determine_type(ASTDiagnoser& diagnoser) {
    const auto type = value->getType();
    // checking if operator is overloaded
    const auto node = type->get_linked_canonical_node(true, false);
    if(node) {
        const auto container = node->get_members_container();
        if(container) {
            const auto func_name = get_overloaded_func_name();
            const auto child = container->child(func_name);
            if(!child || child->kind() != ASTNodeKind::FunctionDecl) {
                diagnoser.error(this) << "expected function with name '" << func_name << "' to overload operator but found none";
                return type;
            }
            const auto func = child->as_function_unsafe();
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