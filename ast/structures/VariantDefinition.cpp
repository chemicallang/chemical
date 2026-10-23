// Copyright (c) Chemical Language Foundation 2025.

#include "VariantDefinition.h"
#include "VariantMember.h"
#include "ast/types/LinkedType.h"
#include "compiler/mangler/NameMangler.h"
#include "ast/base/Value.h"
#include "ast/values/VariantCase.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/utils/GenericUtils.h"
#include "ast/types/GenericType.h"


void VariantDefinition::generate_functions(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ASTNode* returnNode) {
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
    if(!has_destructor && !no_drop && any_member_has_destructor()) {
        has_destructor = true;
        create_def_destructor(allocator, diagnoser, returnNode, false);
    }
}

bool VariantDefinition::requires_destructor() {
    for(const auto var : variables()) {
        auto member = var->as_variant_member();
        if(member->requires_destructor()) {
            return true;
        }
    }
    return false;
}

uint64_t VariantDefinition::byte_size(const TargetData& target) {
    const auto type_size = target.is64Bit ? 4 : 2; // <--- an int type enum is stored inside
    const auto large = largest_member();
    if(!large) return type_size;
    return large->byte_size(target) + type_size;
}

ASTNode *VariantMember::child(unsigned int index) {
    if(index >= values.size()) return nullptr;
    return (values.begin() + index)->second;
}

BaseType* VariantMember::child_type(unsigned int index) {
    const auto c = child(index);
    return c ? c->known_type() : nullptr;
}

bool VariantMember::requires_destructor() {
    for(auto& value : values) {
        if(value.second->type->requires_destructor()) {
            return true;
        }
    }
    return false;
}

bool VariantCaseVariable::is_generic_param() {
    const auto linked = member_param->type->linked_node();
    return linked != nullptr && linked->as_generic_type_param() != nullptr;
}