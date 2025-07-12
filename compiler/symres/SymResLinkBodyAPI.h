// Copyright (c) Chemical Language Foundation 2025.

#pragma once

class SymbolResolver;

class Scope;

void sym_res_link_body(SymbolResolver& resolver, Scope* scope);

[[deprecated]]
void sym_res_link_node_deprecated(SymbolResolver& resolver, ASTNode* node);

[[deprecated]]
void sym_res_link_value_deprecated(SymbolResolver& resolver, Value* value, BaseType* expected_type);

[[deprecated]]
void sym_res_link_type_deprecated(SymbolResolver& resolver, BaseType* type, SourceLocation loc);