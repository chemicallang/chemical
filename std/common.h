// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include "chem_string_view.h"

/**
 * empty chemical string view
 */
const chem::string_view EmptyChemStrView = "";

/**
 * empty string is used in places where we need to return a const string& but no string is present
 * @deprecated
 */
const std::string EmptyString = "";