// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/structures/FunctionDeclaration.h"

struct OverridableFuncHandlingResult {
    // if this is true, it means function couldn't be overloaded
    // because it's access specifier is different from previous one
    bool specifier_mismatch = false;
    // duplicate nodes already present on the map, with same name
    std::vector<ASTNode*> duplicates;
    // any new multi function node, that should be handled
    MultiFunctionNode* new_multi_func_node = nullptr;
};

/**
 * multi function node is created when two functions with same name exist
 * however parameter types may be different, this allows us c++ like behavior of
 * overriding, multi function node only exists during symbol resolution
 */
class MultiFunctionNode : public ASTNode {
public:

    chem::string_view name;
    std::vector<FunctionDeclaration*> functions;

    /**
     * constructor
     */
    constexpr MultiFunctionNode(
        chem::string_view name,
        ASTNode* parent,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::MultiFunctionNode, parent, location), name(name) {

    }

    std::optional<AccessSpecifier> specifier() {
        if(functions.empty()) return std::nullopt;
        return functions.front()->specifier();
    }

    FunctionDeclaration* func_for_call(ASTAllocator& allocator, std::vector<Value*>& args);

};

OverridableFuncHandlingResult handle_name_overload_function(
        ASTAllocator& astAllocator,
        ASTNode* previous_node,
        FunctionDeclaration* declaration
);