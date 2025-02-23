// Copyright (c) Qinetik 2024.

#pragma once

#include "FunctionDeclaration.h"
#include "ExtensionFuncReceiver.h"

class ExtensionFunction : public FunctionDeclaration {
public:

    ExtensionFuncReceiver receiver;

//    ExtensionFunction(
//            LocatedIdentifier identifier,
//            ExtensionFuncReceiver receiver,
//            std::vector<FunctionParam*> params,
//            BaseType* returnType,
//            bool isVariadic,
//            ASTNode* parent_node,
//            SourceLocation location,
//            std::optional<Scope> body = std::nullopt,
//            AccessSpecifier specifier = AccessSpecifier::Internal
//    ) : FunctionDeclaration(
//            identifier,
//            std::move(params),
//            returnType,
//            isVariadic,
//            parent_node,
//            location,
//            std::move(body),
//            specifier,
//            false,
//            ASTNodeKind::ExtensionFunctionDecl
//    ), receiver(std::move(receiver)) {
//
//    }

    ExtensionFunction(
            LocatedIdentifier identifier,
            ExtensionFuncReceiver receiver,
            BaseType* returnType,
            bool isVariadic,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : FunctionDeclaration(
            identifier,
            returnType,
            isVariadic,
            parent_node,
            location,
            specifier,
            false,
            ASTNodeKind::ExtensionFunctionDecl
    ), receiver(std::move(receiver)) {

    }

#ifdef COMPILER_BUILD

    std::vector<llvm::Type *> param_types(Codegen &gen) final;

#endif

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
     * links the signature of the extension function
     */
    void link_signature(SymbolResolver &linker) override;

    /**
     * declare and link
     */
    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

};