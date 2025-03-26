// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>

const char* parseModDecl(char* scope_name, char* mod_name, size_t& scopeSizeOut, size_t& modSizeOut, size_t each_buffer_size, const std::string& filePath);