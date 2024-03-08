// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "InterpretScope.h"
#include "GlobalInterpretScope.h"
#include "Value.h"

InterpretScope::InterpretScope(InterpretScope* parent, GlobalInterpretScope *global, Scope* scope, ASTNode* node) : parent(parent), global(global), codeScope(scope), node(node) {

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
        if(it.second->references() == 1){
            delete it.second;
        } else {
           it.second->decrease_reference();
        }
    }
    values.clear();
}