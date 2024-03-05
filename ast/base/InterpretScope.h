// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <string>
#include "InterpretValue.h"
#include <unordered_map>

class InterpretScope : public InterpretValue {
public:

    void error(const std::string& err) {
        errors.emplace_back(err);
    }

    /**
     * This contains a map between identifiers and its values
     * When a variable is created, the variable sets the identifier in unordered-map
     */
    std::unordered_map<std::string, InterpretValue*> values;

    /**
     * This contains errors that occur during interpretation
     */
    std::vector<std::string> errors;

};