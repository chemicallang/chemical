// Copyright (c) Chemical Language Foundation 2025.

#include "Negative.h"
#include "ast/base/BaseType.h"
#include "IntNumValue.h"
#include "ast/types/IntNType.h"

uint64_t NegativeValue::byte_size(bool is64Bit) {
// TODO check this out
    return value->byte_size(is64Bit);
}

BaseType* NegativeValue::create_type(ASTAllocator& allocator) {
    const auto result = value->create_type(allocator);
    const auto can = result->canonical();
    if(can->kind() == BaseTypeKind::IntN) {
        const auto intN = can->as_intn_type_unsafe();
        return intN->to_signed(allocator);
    }
    return result;
}

Value* NegativeValue::evaluated_value(InterpretScope &scope) {
    const auto eval = value->evaluated_value(scope);
    const auto eval_kind = eval->val_kind();
    if(eval_kind >= ValueKind::IntNStart && eval_kind <= ValueKind::IntNEnd) {
        return pack_by_kind(scope, eval_kind, -((IntNumValue*) eval)->get_num_value(), encoded_location());
    } else if(eval_kind == ValueKind::Double || eval_kind == ValueKind::Float) {
        return pack_by_kind(scope, eval_kind, -get_double_value(eval, eval_kind), encoded_location());
    } else {
        return nullptr;
    }
}

BaseType* NegativeValue::known_type() {
    return value->known_type();
}

bool NegativeValue::primitive() {
    return value->primitive();
}