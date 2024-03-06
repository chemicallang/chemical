// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "GlobalInterpretScope.h"

GlobalInterpretScope::GlobalInterpretScope() : InterpretScope(nullptr, this) {

}

void GlobalInterpretScope::add_error(const std::string &err) {
    errors.emplace_back(err);
}