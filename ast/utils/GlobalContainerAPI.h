// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <optional>
#include "std/chem_string_view.h"

struct GlobalContainer;

struct IffyBase;

class ASTNode;

std::optional<bool> is_condition_enabled(GlobalContainer* container, const chem::string_view& name);

std::optional<bool> is_condition_enabled(GlobalContainer* container, IffyBase* base);