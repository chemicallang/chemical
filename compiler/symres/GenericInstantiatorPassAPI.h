// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include "LinkSignatureAPI.h"

/**
 * symbol resolution but you can pass sym res signature result to be processed
 */
void sym_res_generic_instantiation(SymbolResolver& resolver, Scope* scope, SymResSignatureResult& result);
