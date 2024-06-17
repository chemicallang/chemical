// Copyright (c) Qinetik 2024.

#pragma once

#include "FunctionDeclaration.h"
#include "ExtensionFuncReceiver.h"

class ExtensionFunction : public FunctionDeclaration {
public:

    ExtensionFuncReceiver receiver;

    ExtensionFunction(
            std::string name,
            ExtensionFuncReceiver receiver,
            std::vector<std::unique_ptr<FunctionParam>> params,
            std::unique_ptr<BaseType> returnType,
            bool isVariadic,
            std::optional<LoopScope> body = std::nullopt
    );

#ifdef COMPILER_BUILD

    std::vector<llvm::Type *> param_types(Codegen &gen) override;

#endif

    /**
     * creates value type
     */
    std::unique_ptr<BaseType> create_value_type() override;

    /**
     * requires self, meaning the type must be passed as first argument
     */
    bool has_self_param() override {
        return true;
    }

    /**
     * first param self is not part of params, so index shifts to 1
     */
    unsigned int c_or_llvm_arg_start_index() const override {
        return FunctionDeclaration::c_or_llvm_arg_start_index() + 1;
    }

    /**
     * extension function will add references to extendable members container
     */
    void declare_top_level(SymbolResolver &linker) override;

    /**
     * declare and link
     */
    void declare_and_link(SymbolResolver &linker) override;

};