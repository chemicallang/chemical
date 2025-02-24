// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

class ExtensionFuncReceiver : public BaseFunctionParam {
public:

    ASTNode* parent_node;

    /**
     * constructor
     */
    ExtensionFuncReceiver(
        chem::string_view name,
        BaseType* type,
        ASTNode* parent_node,
        SourceLocation location
    ) : BaseFunctionParam(name, type, nullptr, ASTNodeKind::ExtensionFuncReceiver, location), parent_node(parent_node) {

    }


    ASTNode * parent() final {
        return parent_node;
    }

    ASTNode *child(const chem::string_view &name) final;

    unsigned int calculate_c_or_llvm_index() final;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

};