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
 * pass 1: only visit generic declarations in the file, resolving their master/template bodies.
 * non-generic declarations aren't visited
 * this allows to completely have all generic declarations symbol resolved, before we can begin symbol resolving non-generic declarations
 */
SymResLinkBodyResult sym_res_link_body_generic_decls_pass(SymbolResolver& resolver, Scope* scope, const SymbolRange& range);

/**
 * pass 2: only visit non-generic declarations in the file, skipping over all generic declarations
 */
SymResLinkBodyResult sym_res_link_body_pass(SymbolResolver& resolver, Scope* scope, const SymbolRange& range);