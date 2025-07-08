// Copyright (c) Chemical Language Foundation 2025.

#include "UnionType.h"
#include "ast/structures/VariablesContainer.h"
#include "compiler/SymbolResolver.h"
#include "compiler/symres/LinkSignatureAPI.h"

bool UnionType::link(SymbolResolver &linker, SourceLocation loc) {
    take_variables_from_parsed_nodes(linker);
    sym_res_vars_signature(linker, this);
    if(!name.empty()) {
        linker.declare(name, this);
    }
    return true;
}