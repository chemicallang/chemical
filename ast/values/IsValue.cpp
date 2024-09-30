// Copyright (c) Qinetik 2024.

#include "IsValue.h"
#include "ast/base/ASTNode.h"

IsValue::IsValue(
        Value* value,
        BaseType* type,
        bool is_negating,
        CSTToken* token
) : value(value), type(type), is_negating(is_negating), token(token) {

}

IsValue *IsValue::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<IsValue>()) IsValue(
            value->copy(allocator),
            type->copy(allocator),
            is_negating,
            token
    );
}

bool IsValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    value->link(linker, value);
    type->link(linker);
    return true;
}

std::optional<bool> IsValue::get_comp_time_result() {
    const auto linked = value->linked_node();
    if(linked) {
        bool result;
        const auto param = linked->as_generic_type_param();
        if (param) {
            result = type->is_same(linked->known_type());
            return is_negating ? !result : result;
        }
        const auto alias = linked->as_typealias();
        if(alias) {
            result = type->is_same(linked->known_type());
            return is_negating ? !result : result;
        }
    }
    return std::nullopt;
}