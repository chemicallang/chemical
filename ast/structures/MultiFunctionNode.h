// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

struct OverridableFuncHandlingResult {
    // duplicate nodes already present on the map, with same name
    std::vector<ASTNode*> duplicates;
    // any new multi function node, that should be handled
    MultiFunctionNode* new_multi_func_node;
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

    FunctionDeclaration* func_for_call(ASTAllocator& allocator, std::vector<Value*>& args);

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

};

OverridableFuncHandlingResult handle_name_overload_function(
        const chem::string_view& name,
        ASTNode* previous_node,
        FunctionDeclaration* declaration
);