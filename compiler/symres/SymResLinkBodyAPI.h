// Copyright (c) Chemical Language Foundation 2025.

#pragma once

class SymbolResolver;

class Scope;

void sym_res_link_body(SymbolResolver& resolver, Scope* scope);

[[deprecated]]
void sym_res_link_node_deprecated(SymbolResolver& resolver, ASTNode* node);