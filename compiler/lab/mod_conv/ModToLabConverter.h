// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <ostream>

class ModuleFileData;

/**
 * convert a module file data to build lab string
 */
void convertToBuildLab(const ModuleFileData& data, std::ostream& output);