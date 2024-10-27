// Copyright (c) Qinetik 2024.

#include "ArrayType.h"
#include "ast/base/Value.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/GlobalInterpretScope.h"

bool ArrayType::link(SymbolResolver &linker) {
    const auto type_linked = elem_type->link(linker);
    if(!type_linked) return false;
    if(array_size_value) {
        if(array_size_value->link(linker, array_size_value)) {
            const auto evaluated = array_size_value->evaluated_value(linker.comptime_scope);
            const auto number = evaluated->get_the_number();
            if(number.has_value()) {
                array_size = number.value();
                return true;
            }
        }
        return false;
    }
    return true;
}