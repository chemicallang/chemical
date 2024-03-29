// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include <map>
#include "ast/base/Value.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/statements/VarInit.h"
#include "ast/base/GlobalInterpretScope.h"

class StructDefinition : public ASTNode {
public:

    /**
     * @brief Construct a new StructDeclaration object.
     *
     * @param name The name of the struct.
     * @param fields The members of the struct.
     */
    StructDefinition(
            std::string name,
            std::map<std::string, std::unique_ptr<VarInitStatement>> fields,
            std::map<std::string, std::unique_ptr<FunctionDeclaration>> functions,
            std::optional<std::string> overrides
    );

    void accept(Visitor &visitor) override;

    void declare_top_level(ASTLinker &linker) override;

    StructDefinition *as_struct_def() override;

    FunctionDeclaration *member(const std::string &name);

    bool type_check(InterpretScope &scope);

    void interpret(InterpretScope &scope) override;

    void interpret_scope_ends(InterpretScope &scope) override;

#ifdef COMPILER_BUILD

    std::vector<llvm::Type *> elements_type(Codegen &gen);

    void code_gen(Codegen &gen) override;

#endif

    std::string representation() const override;

    InterpretScope *decl_scope;
    std::string name; ///< The name of the struct.
    std::optional<std::string> overrides;
    std::map<std::string, std::unique_ptr<VarInitStatement>> variables; ///< The members of the struct.
    std::map<std::string, std::unique_ptr<FunctionDeclaration>> functions;
};