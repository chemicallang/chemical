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

InterpretScope::InterpretScope(
        InterpretScope *parent,
        ASTAllocator& allocator,
        GlobalInterpretScope *global
) : parent(parent), allocator(allocator), global(global) {

}

ASTAny* InterpretScope::allocate_released(std::size_t obj_size, std::size_t alignment) {
    const auto ptr = (ASTAny*) (void*) allocator.allocate_released_size(obj_size, alignment);
    allocated.emplace_back(ptr);
    return ptr;
}

void InterpretScope::declare(std::string &name, Value *value) {
    values[name] = value;
}

void InterpretScope::declare(const chem::string_view& name, Value* value) {
    values[name.str()] = value;
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

std::pair<value_iterator, InterpretScope&> InterpretScope::find_value_iterator(const std::string &name) {
    auto found = values.find(name);
    if (found == values.end()) {
        if(parent == nullptr) return { values.end(), *this };
        return parent->find_value_iterator(name);
    } else {
        return { found, *this };
    }
}

void InterpretScope::erase_value(const std::string &name) {
    auto iterator = find_value_iterator(name);
    if(iterator.first == iterator.second.values.end()) {
        std::cerr << ANSI_COLOR_RED << "couldn't locate value " << name << " for removal" << ANSI_COLOR_RESET
                  << std::endl;
#ifdef DEBUG
        print_values();
#endif
    } else {
        iterator.second.values.erase(iterator.first);
    }
}

void InterpretScope::error(std::string& err, ASTAny* any) {
    global->interpret_error(err, any);
}

void InterpretScope::error(std::string_view err, ASTAny* any) {
    global->interpret_error(err, any);
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

InterpretScope::~InterpretScope() {
    for(auto ptr : allocated) {
        ptr->~ASTAny();
    }
}