// Copyright (c) Chemical Language Foundation 2025.

#include "Typealias.h"
#include "ast/base/InterpretScope.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"

BaseType* TypealiasStatement::known_type() {
    return actual_type;
}

uint64_t TypealiasStatement::byte_size(bool is64Bit) {
    return actual_type->byte_size(is64Bit);
}