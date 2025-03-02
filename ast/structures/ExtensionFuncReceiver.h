// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

class ExtensionFuncReceiver : public BaseFunctionParam {
public:

    /**
     * constructor
     */
    constexpr ExtensionFuncReceiver(
        chem::string_view name,
        BaseType* type,
        ASTNode* parent_node,
        SourceLocation location
    ) : BaseFunctionParam(name, type, nullptr, ASTNodeKind::ExtensionFuncReceiver, parent_node, location) {

    }


    ASTNode *child(const chem::string_view &name) final;

    unsigned int calculate_c_or_llvm_index() final;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

};