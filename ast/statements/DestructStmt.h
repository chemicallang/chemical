// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/values/VariableIdentifier.h"

class ExtendableMembersContainerNode;

struct DestructData {
    ExtendableMembersContainerNode* parent_node;
    FunctionDeclaration* destructor_func;
    uint64_t array_size; // 0 if not known
};

class DestructStmt : public ASTNode {
public:

    /**
     * if the statement has brackets destruct[] ptr;
     */
    bool is_array;
    /**
     * the actual identifier / access chain value destruct[array_value] ptr; <--- ptr is the one
     */
    Value* identifier;
    /**
     * array value is the one in brackets like destruct[array_value] ptr;
     */
    Value* array_value;
    /**
     * free function is linked
     */
    FunctionDeclaration* free_func_linked = nullptr;
    /**
     * parent node
     */
    ASTNode* parent_node;

    /**
     * constructor
     */
    DestructStmt(
        Value* array_value,
        Value* value,
        bool is_array,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::DeleteStmt, location), array_value(array_value), identifier(value), is_array(is_array), parent_node(parent_node) {

    }

    DestructData get_data(ASTAllocator& allocator);

    ASTNode* parent() final {
        return parent_node;
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};