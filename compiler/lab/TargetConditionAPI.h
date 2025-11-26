// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "TargetData.h"
#include <optional>
#include "std/chem_string_view.h"

struct IffyBase;

std::optional<bool> is_condition_enabled(TargetData& data, const chem::string_view& name);

std::optional<bool> resolve_target_condition(TargetData& data, IffyBase* base);