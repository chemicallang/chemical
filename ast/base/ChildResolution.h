// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"

class ASTNode;
class ChainValue;
class ChildResolver;

ASTNode* provide_child(ChildResolver* resolver, ChainValue* parent, const chem::string_view& name, ASTNode* type_parent);