// Copyright (c) Chemical Language Foundation 2025.

#include "UnionType.h"
#include "ast/structures/VariablesContainer.h"
#include "compiler/SymbolResolver.h"

bool UnionType::link(SymbolResolver &linker) {
    take_variables_from_parsed_nodes(linker);
    VariablesContainer::link_variables_signature(linker);
    if(!name.empty()) {
        linker.declare(name, this);
    }
    return true;
}