// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <span>
#include <string>
#include "std/chem_string_view.h"
#include "std/chem_string.h"

char** chem_string_cmd_pointers(chem::string& front, const std::span<chem::string>& command_args);

char** chem_string_cmd_pointers(const std::span<chem::string>& command_args);

void free_chem_string_cmd_pointers(char** pointers, size_t size);