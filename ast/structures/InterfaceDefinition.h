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

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    void declare_top_level(SymbolResolver &linker) override {
        linker.current[name] = this;
    }

    bool has_implemented(const std::string& name);

    void set_implemented(const std::string& name, bool impl);

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

    llvm::Type* llvm_type(Codegen &gen) override;

#endif

    std::string representation() const override;

    std::string name; ///< The name of the interface.
    // this unordered map tracks if functions with name have been implemented
    // to prevent duplicate implementations
    std::unordered_map<std::string, bool> implemented;
};