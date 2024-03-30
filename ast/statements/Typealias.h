// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"

class TypealiasStatement : public ASTNode {
public:

    // before equal
    std::unique_ptr<BaseType> from;
    // after equal
    std::unique_ptr<BaseType> to;

    /**
     * @brief Construct a new TypealiasStatement object.
     */
    TypealiasStatement(std::unique_ptr<BaseType> from, std::unique_ptr<BaseType> to);

    void interpret(InterpretScope &scope) override;

    void declare_and_link(ASTLinker &linker) override;

    void accept(Visitor &visitor) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

    std::string representation() const override;

};