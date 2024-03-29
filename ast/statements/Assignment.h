#include <utility>

// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/values/AccessChain.h"
#include "ast/utils/Operation.h"

class AssignStatement : public ASTNode {
public:

    /**
     * @brief Construct a new AssignStatement object.
     *
     * @param identifier The identifier being assigned.
     * @param value The value being assigned to the identifier.
     */
    AssignStatement(
            std::unique_ptr<AccessChain> lhs,
            std::unique_ptr<Value> value,
            Operation assOp
    );

    void accept(Visitor &visitor) override;

    void declare_and_link(ASTLinker &linker) override;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override;
#endif

    void interpret(InterpretScope& scope) override;

    void interpret_scope_ends(InterpretScope &scope) override;

    std::string representation() const override;

private:
    std::unique_ptr<AccessChain> lhs;
    std::unique_ptr<Value> value;
    InterfaceDefinition* definition;
    Operation assOp;
};