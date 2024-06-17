// Copyright (c) Qinetik 2024.

#include "ast/base/ASTNode.h"

class ExtensionFuncReceiver : public BaseFunctionParam {
public:

    /**
     * constructor
     */
    ExtensionFuncReceiver(
        std::string name,
        std::unique_ptr<BaseType> type
    );

    unsigned int calculate_c_or_llvm_index() override;

    void declare_and_link(SymbolResolver &linker) override;

    void accept(Visitor *visitor) override;

    std::string representation() const override;

};