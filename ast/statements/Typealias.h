// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/base/ExtendableAnnotableNode.h"

class TypealiasStatement : public ExtendableAnnotableNode {
public:

    // before equal
    std::string identifier;
    // after equal
    std::unique_ptr<BaseType> actual_type;

    /**
     * @brief Construct a new TypealiasStatement object.
     */
    TypealiasStatement(
            std::string identifier,
            std::unique_ptr<BaseType> actual_type
    );

    TypealiasStatement* as_typealias() override {
        return this;
    }

    std::string ns_node_identifier() override {
        return identifier;
    }

    void interpret(InterpretScope &scope) override;

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    void accept(Visitor *visitor) override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    void code_gen(Codegen &gen) override;

#endif

};