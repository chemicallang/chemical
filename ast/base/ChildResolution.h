// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"

class ASTNode;
class ChildResolver;
class BaseType;

ASTNode* provide_child(const ChildResolver* resolver, Value* parent, const chem::string_view& name, ASTNode* type_parent);

/**
 * resolves a child by the given type (unwrapping runtime / reference / pointer
 * wrappers) on the type's linked container
 */
ASTNode* provide_child(const ChildResolver* resolver, BaseType* type, const chem::string_view& name, ASTNode* type_parent);