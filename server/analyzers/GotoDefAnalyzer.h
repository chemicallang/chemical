// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include "common/Location.h"

class ImportUnit;

class GotoDefAnalyzer {
public:

    Position position;

    GotoDefAnalyzer(Position position);

    std::vector<Location> analyze(ImportUnit* unit);

};