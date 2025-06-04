// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "Range.h"
#include <string>

struct Location {
    Range range;
    std::string path; // the absolute path to the document where this range exists
};