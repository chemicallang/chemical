// Copyright (c) Chemical Language Foundation 2025.

#pragma once

class ASTNode;

class SymbolResolver;

class Scope;

class VariablesContainer;

/**
 * link signature of given scope
 */
void sym_res_signature(SymbolResolver& resolver, Scope* scope);

/**
 * symbol resolve the variable types and default values in the container
 */
void sym_res_vars_signature(SymbolResolver& resolver, VariablesContainer* container);