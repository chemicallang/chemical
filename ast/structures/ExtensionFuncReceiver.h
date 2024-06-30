// Copyright (c) Qinetik 2024.

#include "ast/base/ASTNode.h"

class ExtensionFuncReceiver : public BaseFunctionParam {
public:

    ASTNode* parent_node;

    /**
     * constructor
     */
    ExtensionFuncReceiver(
        std::string name,
        std::unique_ptr<BaseType> type,
        ASTNode* parent_node
    );

    ASTNode * parent() override {
        return parent_node;
    }

    ASTNode *child(const std::string &name) override;

    unsigned int calculate_c_or_llvm_index() override;

    void declare_and_link(SymbolResolver &linker) override;

    void accept(Visitor *visitor) override;

};