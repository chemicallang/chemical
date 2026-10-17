// Copyright (c) Chemical Language Foundation 2025.

#include "PatternMatchExpr.h"
#include "compiler/symres/SymbolResolver.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/types/VoidType.h"
#include "ast/structures/VariantMember.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/values/BoolValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/PointerValue.h"
#include "ast/types/BoolType.h"


VariantMember* PatternMatchExpr::find_member_from_expr(ASTAllocator& allocator, ASTDiagnoser& diagnoser) {
    const auto type = expression->getType();
    if(!type) {
        diagnoser.error("unknown type for expression", expression);
        return nullptr;
    }
    // disallow pointers, only direct or referenced variants are allowed
    const auto linked_node = type->get_linked_canonical_node(true, false);
    if(!linked_node) {
        diagnoser.error("couldn't resolve variant from given expression, expression must be of type variant or reference to a variant", expression);
        return nullptr;
    }
    if(linked_node->kind() == ASTNodeKind::VariantDecl) {
        const auto decl = linked_node->as_variant_def_unsafe();
        const auto found_child = decl->child(member_name);
        if(!found_child) {
            diagnoser.error(this) << "couldn't find member with name '" << member_name << "'";
            return nullptr;
        }
        if(found_child->kind() != ASTNodeKind::VariantMember) {
            diagnoser.error("member is not a variant member", this);
            return nullptr;
        }
        return found_child->as_variant_member_unsafe();
    } else if(linked_node->kind() == ASTNodeKind::GenericVariantDecl) {
        const auto decl = linked_node->as_gen_variant_decl_unsafe();
        const auto found_child = decl->child(member_name);
        if(!found_child) {
            diagnoser.error(this) << "couldn't find member with name '" << member_name << "'";
            return nullptr;
        }
        if(found_child->kind() != ASTNodeKind::VariantMember) {
            diagnoser.error("member is not a variant member", this);
            return nullptr;
        }
        return found_child->as_variant_member_unsafe();
    } else {
        diagnoser.error("linked declaration is not a variant", expression);
        return nullptr;
    }
}

