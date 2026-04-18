// Copyright (c) Chemical Language Foundation 2026.

#pragma once
#include <unordered_map>

class InterfaceDefinition;

class ExtendableMembersContainerNode;

class FunctionDeclaration;

struct TraitImplFuncMapKey {

    // this is always an instantiation
    // its never a master template interface
    InterfaceDefinition* interface;
    // the for struct in an impl
    ExtendableMembersContainerNode* for_;
    // the function
    FunctionDeclaration* func;

    bool operator==(const TraitImplFuncMapKey& other) const {
        return interface == other.interface && for_ == other.for_ && func == other.func;
    }
};

struct TraitImplFuncMapKeyHash {
    std::size_t operator()(const TraitImplFuncMapKey& k) const {
        const auto h1 = std::hash<void*>()(k.interface);
        const auto h2 = std::hash<void*>()(k.for_);
        const auto h3 = std::hash<void*>()(k.func);
        // Combine hashes (simple but decent)
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};