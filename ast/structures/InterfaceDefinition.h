// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/Value.h"

class InterfaceDefinition : public ASTNode {
public:

    /**
     * @brief Construct a new InterfaceDeclaration object.
     *
     * @param name The name of the interface.
     * @param methods The methods declared in the interface.
     */
    InterfaceDefinition(std::string name, std::vector<std::unique_ptr<ASTNode>> members)
            : name(std::move(name)), members(std::move(members)) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    void declare_top_level(ASTLinker &linker) override {
        linker.current[name] = this;
    }

    bool verify(InterpretScope &scope, const std::string& name, const std::vector<std::unique_ptr<VarInitStatement>>& members) {
        scope.error("Not implemented verifying struct definition with interface");
        return false;
    }

    std::string representation() const override {
        std::string ret("interface " + name + " {\n");
        auto i = 0;
        while(i < members.size()) {
            ret.append(members[i]->representation());
            if(i < members.size() - 1) {
                ret.append(1, '\n');
            }
            i++;
        }
        ret.append("\n}");
        return ret;
    }

private:
    std::string name; ///< The name of the interface.
    std::vector<std::unique_ptr<ASTNode>> members; ///< The methods declared in the interface.
};