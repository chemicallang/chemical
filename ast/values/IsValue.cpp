// Copyright (c) Qinetik 2024.

#include "IsValue.h"
#include "ast/base/ASTNode.h"
#include "ast/base/InterpretScope.h"
#include "ast/values/BoolValue.h"
#include "ast/values/NullValue.h"

IsValue::IsValue(
        Value* value,
        BaseType* type,
        bool is_negating,
        SourceLocation location
) : value(value), type(type), is_negating(is_negating), location(location) {

}

IsValue *IsValue::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<IsValue>()) IsValue(
            value->copy(allocator),
            type->copy(allocator),
            is_negating,
            location
    );
}

bool IsValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    value->link(linker, value);
    type->link(linker);
    return true;
}

Value* IsValue::evaluated_value(InterpretScope &scope) {
    const auto result = get_comp_time_result();
    if(result.has_value()) {
        return new (scope.allocate<BoolValue>()) BoolValue(result.value(), location);
    } else {
        return new (scope.allocate<NullValue>()) NullValue(location);
    }
}

std::optional<bool> IsValue::get_comp_time_result() {
    const auto linked = value->linked_node();
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