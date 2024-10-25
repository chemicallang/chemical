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

    CSTToken *cst_token() final {
        return token;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::ExtensionFuncReceiver;
    }

    ASTNode * parent() final {
        return parent_node;
    }

    ASTNode *child(const std::string &name) final;

    unsigned int calculate_c_or_llvm_index() final;

    void declare_and_link(SymbolResolver &linker) final;

    void accept(Visitor *visitor) final;

};