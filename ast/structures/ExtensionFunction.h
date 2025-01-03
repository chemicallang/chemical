// Copyright (c) Qinetik 2024.

#pragma once

#include "FunctionDeclaration.h"
#include "ExtensionFuncReceiver.h"

class ExtensionFunction : public FunctionDeclaration {
public:

    ExtensionFuncReceiver receiver;

    ExtensionFunction(
            LocatedIdentifier identifier,
            ExtensionFuncReceiver receiver,
            std::vector<FunctionParam*> params,
            BaseType* returnType,
            bool isVariadic,
            ASTNode* parent_node,
            SourceLocation location,
            std::optional<LoopScope> body = std::nullopt,
            AccessSpecifier specifier = AccessSpecifier::Internal
    );

#ifdef COMPILER_BUILD

    std::vector<llvm::Type *> param_types(Codegen &gen) final;

#endif

    ASTNodeKind kind() final {
        return ASTNodeKind::ExtensionFunctionDecl;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    ExtensionFunction* as_extension_func() final {
        return this;
    }

    /**
     * creates value type
     */
    BaseType* create_value_type(ASTAllocator& allocator) final;

//    hybrid_ptr<BaseType> get_value_type() final;

    /**
     * all extension functions require self, because they are extensions on self, which is the receiver
     */
    BaseFunctionParam *get_self_param() final {
        return &receiver;
    }

    /**
     * first param self is not part of params, so index shifts to 1
     */
    unsigned int c_or_llvm_arg_start_index() final {
        return FunctionDeclaration::c_or_llvm_arg_start_index() + 1;
    }

    /**
     * extension function will add references to extendable members container
     */
    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    /**
     * declare and link
     */
    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

};