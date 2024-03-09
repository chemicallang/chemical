// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "InterpretScope.h"
#include "GlobalInterpretScope.h"
#include "Value.h"

InterpretScope::InterpretScope(InterpretScope* parent, GlobalInterpretScope *global, Scope* scope, ASTNode* node) : parent(parent), global(global), codeScope(scope), node(node) {

}

std::pair<bool, std::unordered_map<std::string, Value *>::iterator> InterpretScope::find(const std::string& value) {
    // try to find the pointer of the value
    auto currentScope = this;
    while (currentScope != nullptr) {
        auto pointer = currentScope->values.find(value);
        if (pointer != currentScope->values.end()) {
            return {true, std::move(pointer)};
        }
        currentScope = currentScope->parent;
    }
    return {false, values.end()};
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
        it.second->scope_ends();
    }
    values.clear();
}