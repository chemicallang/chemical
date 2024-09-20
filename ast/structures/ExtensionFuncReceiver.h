// Copyright (c) Qinetik 2024.

#include "ast/base/ASTNode.h"

class ExtensionFuncReceiver : public BaseFunctionParam {
public:

    ASTNode* parent_node;
    CSTToken* token;

    /**
     * constructor
     */
    ExtensionFuncReceiver(
        std::string name,
        BaseType* type,
        ASTNode* parent_node,
        CSTToken* token
    );

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::ExtensionFuncReceiver;
    }

    ASTNode * parent() override {
        return parent_node;
    }

    ASTNode *child(const std::string &name) override;

    unsigned int calculate_c_or_llvm_index() override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    void accept(Visitor *visitor) override;

};