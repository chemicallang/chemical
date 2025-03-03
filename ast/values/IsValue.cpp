// Copyright (c) Chemical Language Foundation 2025.

#include "IsValue.h"
#include "ast/base/ASTNode.h"
#include "ast/base/InterpretScope.h"
#include "ast/values/BoolValue.h"
#include "ast/values/TypeInsideValue.h"
#include "ast/values/NullValue.h"

bool IsValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    value->link(linker, value);
    type->link(linker);
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
        const auto param = linked->as_generic_type_param();
        if (param) {
            const auto kt = linked->known_type();
            if(kt) {
                const auto result = type->is_same(kt->pure_type());
                return is_negating ? !result : result;
            } else {
                return std::nullopt;
            }
        }
        const auto alias = linked->as_typealias();
        if(alias) {
            const auto result = type->is_same(linked->known_type());
            return is_negating ? !result : result;
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
            const auto result = type->is_same(typeInside->type->pure_type());
            return is_negating ? !result : result;
        }
    }
    return get_from_node(type, value->linked_node(), is_negating);
}