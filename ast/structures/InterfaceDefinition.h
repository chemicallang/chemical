// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/Value.h"
#include "MembersContainer.h"
#include <map>

class InterfaceDefinition : public MembersContainer {
public:

    /**
     * @brief Construct a new InterfaceDeclaration object.
     *
     * @param name The name of the interface.
     * @param methods The methods declared in the interface.
     */
    InterfaceDefinition(
            std::string name
    );

    InterfaceDefinition *as_interface_def() override {
        return this;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void declare_top_level(SymbolResolver &linker) override;

    std::unique_ptr<BaseType> create_value_type() override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

    llvm::Type* llvm_type(Codegen &gen) override;

#endif

    std::string representation() const override;

    std::string name; ///< The name of the interface.
};