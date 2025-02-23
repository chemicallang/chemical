// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class ExtensionFuncReceiver : public BaseFunctionParam {
public:

    ASTNode* parent_node;
    SourceLocation location;

    /**
     * constructor
     */
    ExtensionFuncReceiver(
        chem::string_view name,
        BaseType* type,
        ASTNode* parent_node,
        SourceLocation location
    ) : BaseFunctionParam(name, type, nullptr, ASTNodeKind::ExtensionFuncReceiver), parent_node(parent_node), location(location) {

    }

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNode * parent() final {
        return parent_node;
    }

    ASTNode *child(const chem::string_view &name) final;

    unsigned int calculate_c_or_llvm_index() final;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void accept(Visitor *visitor) final;

};