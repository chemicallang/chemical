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
            std::vector<FunctionParam*> params,
            BaseType* returnType,
            bool isVariadic,
            ASTNode* parent_node,
            CSTToken* token,
            std::optional<LoopScope> body = std::nullopt,
            AccessSpecifier specifier = AccessSpecifier::Internal
    );

#ifdef COMPILER_BUILD

    std::vector<llvm::Type *> param_types(Codegen &gen) override;

#endif

    ASTNodeKind kind() override {
        return ASTNodeKind::ExtensionFunctionDecl;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    ExtensionFunction * as_extension_func() override {
        return this;
    }

    std::string func_opt_name() override {
        return name;
    }

    /**
     * creates value type
     */
    BaseType* create_value_type(ASTAllocator& allocator) override;

//    hybrid_ptr<BaseType> get_value_type() override;

    /**
     * all extension functions require self, because they are extensions on self, which is the receiver
     */
    BaseFunctionParam *get_self_param() override {
        return &receiver;
    }

    /**
     * first param self is not part of params, so index shifts to 1
     */
    unsigned int c_or_llvm_arg_start_index() override {
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