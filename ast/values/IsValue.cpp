// Copyright (c) Qinetik 2024.

#include "IsValue.h"
#include "ast/base/ASTNode.h"

IsValue::IsValue(
        std::unique_ptr<Value> value,
        std::unique_ptr<BaseType> type
) : value(std::move(value)), type(std::move(type)) {

}

IsValue *IsValue::copy() {
    return new IsValue(
            std::unique_ptr<Value>(value->copy()),
            std::unique_ptr<BaseType>(type->copy())
    );
}

void IsValue::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) {
    value->link(linker, value);
    type->link(linker, type);
}

std::optional<bool> IsValue::get_comp_time_result() {
    const auto linked = value->linked_node();
    if(linked) {
        const auto param = linked->as_generic_type_param();
        if (param) {
            return linked->known_type()->is_same(type.get());
        }
        const auto alias = linked->as_typealias();
        if(alias) {
            return linked->known_type()->is_same(type.get());
        }
    }
    return std::nullopt;
}