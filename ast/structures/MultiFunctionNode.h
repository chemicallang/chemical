// Copyright (c) Qinetik 2024.

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

    std::string name;
    std::vector<FunctionDeclaration*> functions;

    /**
     * constructor
     */
    explicit MultiFunctionNode(std::string name) : ASTNode(ASTNodeKind::MultiFunctionNode), name(std::move(name)) {

    }

    SourceLocation encoded_location() final;

    FunctionDeclaration* func_for_call(ASTAllocator& allocator, std::vector<Value*>& args);

    void accept(Visitor *visitor) final {
        // don't do anything
    }

    ASTNode *parent() final {
        return nullptr;
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

};

OverridableFuncHandlingResult handle_name_overload_function(
        const chem::string_view& name,
        ASTNode* previous_node,
        FunctionDeclaration* declaration
);