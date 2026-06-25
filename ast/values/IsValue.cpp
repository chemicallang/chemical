// Copyright (c) Chemical Language Foundation 2025.

#include "IsValue.h"
#include "ast/base/ASTNode.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/BoolValue.h"
#include "ast/values/TypeInsideValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/values/StructValue.h"
#include "ast/values/NullValue.h"

Value* IsValue::evaluated_value(InterpretScope &scope) {
    auto& typeBuilder = scope.global->typeBuilder;
    const auto result = get_comp_time_result();
    if(result.has_value()) {
        return new (scope.allocate<BoolValue>()) BoolValue(result.value(), typeBuilder.getBoolType(), encoded_location());
    }
    // Runtime variant member check: handle `value is VariantType.Member` expressions
    if(value) {
        auto evalVal = value->evaluated_value(scope);
        if(evalVal && evalVal->val_kind() == ValueKind::StructValue) {
            auto structVal = evalVal->as_struct_value_unsafe();
            auto it = scope.global->variant_member_index_map.find(structVal);
            if(it != scope.global->variant_member_index_map.end()) {
                auto typeLinked = type->get_linked_canonical_node(true, false);
                if(typeLinked && typeLinked->kind() == ASTNodeKind::VariantMember) {
                    auto member = typeLinked->as_variant_member_unsafe();
                    auto variantDef = member->parent();
                    int64_t memberIndex = variantDef->direct_child_index(member->name_view());
                    bool matches = it->second == memberIndex;
                    bool result = is_negating ? !matches : matches;
                    return new (scope.allocate<BoolValue>()) BoolValue(
                        result, typeBuilder.getBoolType(), encoded_location()
                    );
                }
            }
        }
    }
    return new (scope.allocate<NullValue>()) NullValue(typeBuilder.getNullPtrType(), encoded_location());
}

std::optional<bool> get_from_node(BaseType* type, ASTNode* linked, bool is_negating) {
    if(linked) {
        switch(linked->kind()) {
            case ASTNodeKind::GenericTypeParam:{
                const auto param = linked->as_generic_type_param_unsafe();
                if (param) {
                    const auto kt = linked->known_type();
                    if(kt) {
                        const auto result = type->is_same(kt->canonical());
                        return is_negating ? !result : result;
                    } else {
                        return std::nullopt;
                    }
                }
                break;
            }
            case ASTNodeKind::TypealiasStmt:{
                const auto alias = linked->as_typealias_unsafe();
                if(alias) {
                    const auto result = type->is_same(linked->known_type());
                    return is_negating ? !result : result;
                }
                break;
            }
            case ASTNodeKind::StructDecl: {
                const auto other_linked = type->get_direct_linked_node();
                if(!other_linked) {
                    return false;
                }
                if(linked == other_linked) {
                    return true;
                } else {
                    return linked->as_struct_def_unsafe()->extends_node(other_linked);
                }
            }
            default:
                break;
        }
    }
    return std::nullopt;
}

std::optional<bool> IsValue::get_comp_time_result() {
    if(value->kind() == ValueKind::TypeInsideValue) {
        const auto typeInside = (TypeInsideValue*) value;
        if(typeInside->type->kind() == BaseTypeKind::Linked) {
            return get_from_node(type, typeInside->type->linked_node(), is_negating);
        } else {
            const auto result = type->is_same(typeInside->type->canonical());
            return is_negating ? !result : result;
        }
    }
    return get_from_node(type, value->linked_node(), is_negating);
}