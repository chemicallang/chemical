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

Value* PatternMatchExpr::evaluated_value(InterpretScope &scope) {
    // Evaluate the expression being matched (e.g. the variant value)
    auto evalExpr = expression->evaluated_value(scope);
    // the matched expression may be a reference stored inside a struct member
    // (e.g. h.value where value : &Variant) — dereference it to get the variant
    if(evalExpr && evalExpr->val_kind() == ValueKind::PointerValue) {
        auto deref = ((PointerValue*) evalExpr)->deref(scope, encoded_location(), evalExpr);
        if(deref) {
            evalExpr = deref;
        }
    }
    if(evalExpr && evalExpr->val_kind() == ValueKind::StructValue && member) {
        auto structVal = evalExpr->as_struct_value_unsafe();
        int64_t structMemberIndex = structVal->get_variant_member_index();
        if(structMemberIndex >= 0) {
            auto variantDef = member->parent();
            int64_t memberIndex = variantDef->direct_child_index(member->name_view());
            bool matches = structMemberIndex == memberIndex;
            return new (scope.allocate<BoolValue>()) BoolValue(
                matches,
                scope.global->typeBuilder.getBoolType(),
                encoded_location()
            );
        }
        // Fallback: determine variant member from struct's values vs member params.
        // This handles copies (where the pointer-based map lookup fails).
        if(member && !member->values.empty()) {
            auto variantDef = member->parent();
            int64_t memberIndex = variantDef->direct_child_index(member->name_view());
            if(memberIndex >= 0) {
                bool allParamsMatch = true;
                for(auto& [paramName, param] : member->values) {
                    if(structVal->values.find(paramName) == structVal->values.end()) {
                        allParamsMatch = false;
                        break;
                    }
                }
                if(allParamsMatch) {
                    return new (scope.allocate<BoolValue>()) BoolValue(
                        true,
                        scope.global->typeBuilder.getBoolType(),
                        encoded_location()
                    );
                }
            }
        }
    }
    return new (scope.allocate<BoolValue>()) BoolValue(
        false,
        scope.global->typeBuilder.getBoolType(),
        encoded_location()
    );
}