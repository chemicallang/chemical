// Copyright (c) Qinetik 2024.

#include "ast/base/BaseType.h"
#include "ast/base/InterpretScope.h"
#include "NotValue.h"
#include "BoolValue.h"
#include "NullValue.h"

bool NotValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    return value->link(linker, value);
}

bool NotValue::primitive() {
    return false;
}

BaseType* NotValue::create_type(ASTAllocator &allocator) {
    return value->create_type(allocator);
}

Value* NotValue::evaluated_value(InterpretScope &scope) {
    const auto val = value->evaluated_value(scope);
    if(val->val_kind() == ValueKind::Bool) {
        return new (scope.allocate<BoolValue>()) BoolValue(!val->as_bool_unsafe()->value, location);
    } else {
        scope.error("couldn't evaluate as value didn't return a boolean value", this);
        return new (scope.allocate<NullValue>()) NullValue(location);
    };
}

//hybrid_ptr<BaseType> NotValue::get_base_type() {
//    return value->get_base_type();
//}

BaseType* NotValue::known_type() {
    return value->known_type();
}