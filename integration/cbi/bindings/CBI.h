// Copyright (c) Chemical Language Foundation 2025.

#pragma once


#include <string>
#include <span>
#include "std/chem_string_view.h"
#include <unordered_map>

void prepare_cbi_maps(std::unordered_map<std::string_view, std::span<const std::pair<chem::string_view, void*>>>& map);