// Copyright (c) Chemical Language Foundation 2025.

#include "PatternMatchExpr.h"
#include "compiler/symres/SymbolResolver.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/types/VoidType.h"
#include "ast/structures/VariantMember.h"

bool PatternMatchExpr::link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) {
    if(!expression->link(linker, expression, nullptr)) {
        return false;
    }
    const auto type = expression->create_type(linker.allocator);
    if(!type) {
        linker.error("couldn't resolve linked declaration", expression);
        return false;
    }
    const auto linked_node = type->get_direct_linked_canonical_node();
    if(!linked_node) {
        linker.error("couldn't resolve linked declaration", expression);
        return false;
    }
    if(linked_node->kind() != ASTNodeKind::VariantDecl) {
        linker.error("linked declaration is not a variant", expression);
        return false;
    }
    const auto decl = linked_node->as_variant_def_unsafe();
    const auto child_member = decl->child(member_name);
    if(child_member->kind() != ASTNodeKind::VariantMember) {
        linker.error("member is not a variant member", this);
        return false;
    }
    const auto member = child_member->as_variant_member_unsafe();
    for(const auto nameId : param_names) {
        auto found = member->values.find(nameId->identifier);
        if(found == member->values.end()) {
            linker.error("couldn't find parameter in variant member", nameId);
        } else {
            nameId->member_param = found->second;
            // we declare this id, so anyone can link with it
            linker.declare(nameId->identifier, nameId);
        }
    }
    auto& elseVal = elseExpression.value;
    if (elseVal && !elseVal->link(linker, elseVal, nullptr)) {
        return false;
    }
}

BaseType* PatternMatchExpr::create_type(ASTAllocator &allocator) {
    return new (allocator.allocate<VoidType>()) VoidType();
}