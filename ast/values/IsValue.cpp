// Copyright (c) Chemical Language Foundation 2025.

#include "IsValue.h"
#include "ast/base/ASTNode.h"
#include "ast/base/InterpretScope.h"
#include "ast/values/BoolValue.h"
#include "ast/values/TypeInsideValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/values/NullValue.h"

bool IsValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    value->link(linker, value);
    type.link(linker);
    return true;
}

Value* IsValue::evaluated_value(InterpretScope &scope) {
    const auto result = get_comp_time_result();
    if(result.has_value()) {
        return new (scope.allocate<BoolValue>()) BoolValue(result.value(), encoded_location());
    } else {
        return new (scope.allocate<NullValue>()) NullValue(encoded_location());
    }
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