// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/Value.h"
#include <map>

class InterfaceDefinition : public ASTNode {
public:

    /**
     * @brief Construct a new InterfaceDeclaration object.
     *
     * @param name The name of the interface.
     * @param methods The methods declared in the interface.
     */
    InterfaceDefinition(
            std::string name,
            std::map<std::string, std::unique_ptr<StructMember>> variables,
            std::map<std::string, std::unique_ptr<FunctionDeclaration>> functions
    );

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    void declare_top_level(SymbolResolver &linker) override {
        linker.current[name] = this;
    }

    bool verify(InterpretScope &scope, const std::string& name, const std::vector<std::unique_ptr<VarInitStatement>>& members) {
        scope.error("Not implemented verifying struct definition with interface");
        return false;
    }

    std::string representation() const override;

    std::string name; ///< The name of the interface.
    std::map<std::string, std::unique_ptr<StructMember>> variables; ///< The members of the struct.
    std::map<std::string, std::unique_ptr<FunctionDeclaration>> functions;
};