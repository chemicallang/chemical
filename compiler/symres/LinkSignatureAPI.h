// Copyright (c) Chemical Language Foundation 2025.

#pragma once

class ASTNode;

class SymbolResolver;

class Scope;

class VariablesContainer;

/**
 * must run before link signature
 */
void sym_res_before_signature(SymbolResolver& resolver, Scope* scope);

/**
 * link signature of given scope
 */
void sym_res_signature(SymbolResolver& resolver, Scope* scope);