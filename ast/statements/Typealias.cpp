// Copyright (c) Chemical Language Foundation 2025.

#include "Typealias.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/InterpretScope.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"

ASTNode* TypealiasStatement::child(const chem::string_view &name) {
    const auto linked = actual_type->linked_node();
    return linked ? linked->child(name) : nullptr;
}

BaseType* TypealiasStatement::known_type() {
    return actual_type;
}

uint64_t TypealiasStatement::byte_size(bool is64Bit) {
    return actual_type->byte_size(is64Bit);
}