// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/base/ExtendableAnnotableNode.h"

class TypealiasStatement : public ExtendableAnnotableNode {
public:

    // before equal
    std::string from;
    // after equal
    std::unique_ptr<BaseType> to;

    /**
     * @brief Construct a new TypealiasStatement object.
     */
    TypealiasStatement(std::string from, std::unique_ptr<BaseType> to);

    TypealiasStatement* as_typealias() override {
        return this;
    }

    void interpret(InterpretScope &scope) override;

    void declare_and_link(SymbolResolver &linker) override;

    void accept(Visitor *visitor) override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    void code_gen(Codegen &gen) override;

#endif

};