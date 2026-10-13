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
    if(!has_def_constructor && !is_extern() && all_members_def_constructible()) {
        if(create_def_constructor_checking(allocator, diagnoser, name_view(), returnNode)) {
            if (!has_constructor) {
                attrs.is_direct_init = true;
            }
        }
    }
    if(!has_destructor && any_member_has_destructor()) {
        has_destructor = true;
        create_def_destructor(allocator, diagnoser, returnNode, is_extern());
    }
}
