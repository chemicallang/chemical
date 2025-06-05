// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <span>
#include <string>
#include "std/chem_string_view.h"

char** convert_to_pointers(const std::vector<std::string> &command_args);

char** convert_to_pointers(const std::span<chem::string_view> &command_args);

void free_pointers(char** pointers, size_t size);