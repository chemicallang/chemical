// Copyright (c) Chemical Language Foundation 2025.

#include "StructMember.h"
#include "StructDefinition.h"
#include "FunctionDeclaration.h"
#include "ast/types/LinkedType.h"
#include "ast/types/StructType.h"
#include "InterfaceDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/types/VoidType.h"
#include "ast/types/PointerType.h"
#include "ast/types/GenericType.h"
#include "ast/values/NullValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/StructMemberInitializer.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/statements/Return.h"
#include "ast/structures/Scope.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericTypeParameter.h"
#include "UnnamedStruct.h"
#include "std/except.h"


void StructDefinition::generate_functions(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ASTNode* returnNode) {
    bool has_constructor = false;
    bool has_def_constructor = false;
    bool has_destructor = false;
    for(auto& func : non_gen_range()) {
        if(func->is_constructor_fn()) {
            has_constructor = true;
            if(!func->has_explicit_params()) {
                has_def_constructor = true;
            }
            func->ensure_constructor(allocator, diagnoser, returnNode);
        }
        if(func->is_delete_fn()) {
            func->ensure_destructor(allocator, diagnoser, returnNode);
            has_destructor = true;
        }
    }
    if(!is_tuple_struct() && !has_def_constructor && !is_extern() &&
       all_members_def_constructible()) {
        if(create_def_constructor_checking(allocator, diagnoser, name_view(), returnNode)) {
            if (!has_constructor) {
                attrs.is_direct_init = true;
            }
        }
    }
    if(!has_destructor && !no_drop && any_member_has_destructor()) {
        has_destructor = true;
        create_def_destructor(allocator, diagnoser, returnNode, is_extern());
    }
}

void StructDefinition::create_tuple_struct_constructor(ASTAllocator& allocator,
                                                       AccessSpecifier specifier) {
    const auto loc = encoded_location();
    auto* return_type = new (allocator.allocate<LinkedType>()) LinkedType(this);
    auto* ctor = new (allocator.allocate<FunctionDeclaration>())
        FunctionDeclaration("make", {return_type, loc}, false, this, loc, specifier);
    ctor->set_constructor_fn(true);

    // In a generic master, the constructor body must spell the generic
    // instance (`Pair<T>`) so generic instantiation rewrites the literal to
    // the concrete struct instead of leaving the master type in the body.
    BaseType* body_type = return_type;
    if (generic_parent != nullptr) {
        std::vector<TypeLoc> type_args;
        type_args.reserve(generic_parent->generic_params.size());
        for (auto* param : generic_parent->generic_params) {
            type_args.emplace_back(
                new (allocator.allocate<LinkedType>()) LinkedType(param), loc);
        }
        auto* generic_ref = new (allocator.allocate<LinkedType>())
            LinkedType(generic_parent);
        body_type = new (allocator.allocate<GenericType>())
            GenericType(generic_ref, std::move(type_args),
                        generic_parent->lifetime_params);
    }

    auto* tuple_value = new (allocator.allocate<StructValue>())
        StructValue(body_type, this, this, loc);

    unsigned index = 0;
    for (auto* member : variables()) {
        auto* field = member != nullptr ? member->as_struct_member_unsafe() : nullptr;
        if (field == nullptr) continue;
        auto* param = new (allocator.allocate<FunctionParam>())
            FunctionParam(field->name, field->type, index, nullptr, false, ctor, loc);
        ctor->params.emplace_back(param);
        auto* identifier = new (allocator.allocate<VariableIdentifier>())
            VariableIdentifier(field->name, const_cast<BaseType*>(field->type.getType()), loc);
        identifier->linked = param;
        tuple_value->values.emplace(field->name,
                                    StructMemberInitializer(field->name, identifier));
        index++;
    }

    auto* ret = new (allocator.allocate<ReturnStatement>())
        ReturnStatement(tuple_value, ctor, loc);
    ctor->body.emplace(ctor, loc);
    ctor->body->nodes.emplace_back(ret);
    insert_func(ctor);
}
