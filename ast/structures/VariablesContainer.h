// Copyright (c) Qinetik 2024.

#pragma once

#include "ordered_map.h"
#include "BaseDefMember.h"
#include <string>
#include <memory>

class VariablesContainer {
public:

    tsl::ordered_map<std::string, std::unique_ptr<BaseDefMember>> variables;

    BaseDefMember* largest_member();

};