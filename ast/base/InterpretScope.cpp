// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "InterpretScope.h"
#include "GlobalInterpretScope.h"
#include "Value.h"
#include <iostream>
#include "ast/structures/Scope.h"

#define ANSI_COLOR_RED     "\x1b[91m"
#define ANSI_COLOR_RESET   "\x1b[0m"

InterpretScope::InterpretScope(InterpretScope *parent, GlobalInterpretScope *global)
        : parent(parent), global(global){}

void InterpretScope::declare(const std::string &name, Value *value) {
    values[name] = value;
}

Value *InterpretScope::find_value(const std::string &name) {
    auto found = values.find(name);
    if (found == values.end()) {
        if(parent == nullptr) return nullptr;
        return parent->find_value(name);
    } else {
        return found->second;
    }
}

std::pair<value_iterator, value_map&> InterpretScope::find_value_iterator(const std::string &name) {
    auto found = values.find(name);
    if (found == values.end()) {
        if(parent == nullptr) return {values.end(), values};
        return parent->find_value_iterator(name);
    } else {
        return {found, values};
    }
}

void InterpretScope::erase_value(const std::string &name) {
    auto iterator = find_value_iterator(name);
    if(iterator.first == iterator.second.end()) {
        std::cerr << ANSI_COLOR_RED << "couldn't locate value " << name << " for removal" << ANSI_COLOR_RESET
                  << std::endl;
#ifdef DEBUG
        print_values();
#endif
    } else {
        iterator.second.erase(iterator.first);
    }
}

void InterpretScope::error(const std::string &err) {
    global->add_error(err);
}

void InterpretScope::print_values() {
    std::cout << "Values:" << std::endl;
    for (auto const &value: values) {
        std::cout << value.first << " : " << value.second->representation() << std::endl;
    }
    if(parent != nullptr) {
        std::cout << "Parent ";
        parent->print_values();
    }
}

void InterpretScope::clean() {
    for(auto& value : values) {
        delete value.second;
    }
    values.clear();
}

InterpretScope::~InterpretScope() {
    InterpretScope::clean();
}