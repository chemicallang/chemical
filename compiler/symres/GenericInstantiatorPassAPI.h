// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include "LinkSignatureAPI.h"

/**
 * result can contain things that can be passed to next processor in pipeline
 */
struct GenInstSignatureResult {

    bool has_errors;

    std::vector<Diag> diagnostics;

};

/**
 * symbol resolution but you can pass sym res signature result to be processed
 * range is used to populate the local table with file-private symbols
 * diagnostics are collected from the pass's own diagnoser
 */
GenInstSignatureResult sym_res_generic_instantiation(SymbolResolver& resolver, Scope* scope, SymResSignatureResult& result, const SymbolRange& range);
