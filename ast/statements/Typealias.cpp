// Copyright (c) Chemical Language Foundation 2025.

#include "Typealias.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/InterpretScope.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"

void TypealiasStatement::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.declare_node(name_view(), this, specifier(), false);
}

void TypealiasStatement::link_signature(SymbolResolver &linker) {
    actual_type->link(linker);
}

void TypealiasStatement::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    if(!is_top_level()) {
        linker.declare_node(name_view(), this, specifier(), false);
        actual_type->link(linker);
    }
}

BaseType* TypealiasStatement::known_type() {
    return actual_type;
}

uint64_t TypealiasStatement::byte_size(bool is64Bit) {
    return actual_type->byte_size(is64Bit);
}