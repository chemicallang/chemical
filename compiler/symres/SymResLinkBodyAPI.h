// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "LinkSignatureAPI.h"

class SymbolResolver;

class Scope;

/**
 * result for the link body pass — diagnostics and error status
 */
struct SymResLinkBodyResult {

    bool has_errors;

    std::vector<Diag> diagnostics;

};

void sym_res_link_body(SymbolResolver& resolver, Scope* scope);

/**
 * symbol resolve link body using the class's own Diagnoser, SymbolTable, and GenericInstantiatorAPI.
 * range is used to populate the local table with file-private symbols.
 */
SymResLinkBodyResult sym_res_link_body_pass(SymbolResolver& resolver, Scope* scope, const SymbolRange& range);