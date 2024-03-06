// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "InterpretScope.h"
#include "GlobalInterpretScope.h"
#include "Value.h"

InterpretScope::InterpretScope(InterpretScope* parent, GlobalInterpretScope *global) : parent(parent), global(global) {

}

void InterpretScope::printAllValues() {
    std::cout << "ScopeValues:" << std::endl;
    for(auto const& value : values) {
        std::cout << value.first << " : " << value.second << std::endl;
    }
}

void InterpretScope::error(const std::string &err) {
    global->add_error(err);
}

InterpretScope::~InterpretScope() {
    // delete computed values
    for (const auto& it : values) {
        if (it.second->delete_value()) {
            delete it.second;
        }
    }
    values.clear();
}