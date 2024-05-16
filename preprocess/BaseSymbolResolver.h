// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <string>
#include <unordered_map>

template<typename Node>
class BaseSymbolResolver {
public:

    /**
     * when traversing nodes, a node can declare itself on the map
     * this is vector of scopes, the last scope is current scope
     */
    std::vector<std::unordered_map<std::string, Node*>> current = {{}};

    /**
     * when a scope beings, this should be called
     * it would put a unordered_map on current vector
     */
    void scope_start() {
        current.emplace_back();
    }

    /**
     * when a scope ends, this should be called
     * it would pop a scope map from the current vector
     */
    void scope_end() {
        current.pop_back();
    }

    /**
     * find a symbol on current symbol map
     */
    Node *find(const std::string &name) {
        int i = current.size() - 1;
        std::unordered_map<std::string, Node*> *last;
        while (i >= 0) {
            last = &current[i];
            auto found = last->find(name);
            if (found != last->end()) {
                return found->second;
            }
            i--;
        }
        return nullptr;
    }

};