// Copyright (c) Qinetik 2024.

#include "Negative.h"
#include "ast/base/BaseType.h"

uint64_t NegativeValue::byte_size(bool is64Bit) {
// TODO check this out
    return value->byte_size(is64Bit);
}

bool NegativeValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    return value->link(linker, value);
}

BaseType* NegativeValue::create_type(ASTAllocator& allocator) {
    return value->create_type(allocator);
}

//hybrid_ptr<BaseType> NegativeValue::get_base_type() {
//    return value->get_base_type();
//}

BaseType* NegativeValue::known_type() {
    return value->known_type();
}

bool NegativeValue::primitive() {
    return value->primitive();
}