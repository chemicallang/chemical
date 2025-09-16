// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"

class ASTNode;
class ChainValue;

ASTNode* provide_child(ChainValue* parent, const chem::string_view& name);