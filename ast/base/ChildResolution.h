// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"

class ASTNode;
class ChildResolver;

ASTNode* provide_child(ChildResolver* resolver, Value* parent, const chem::string_view& name, ASTNode* type_parent);