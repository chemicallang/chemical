// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string_view>
#include <string>

class BasicBuildContext;

/**
 * serializes the context (executables and modules) into a string
 */
std::string labBuildContext_toJsonStr(BasicBuildContext& context, bool format = false);

/**
 * gets the modules and executables from the given json
 * @return true if no error occurred, false otherwise
 */
bool labBuildContext_fromJson(BasicBuildContext& context, std::string_view jsonContent);