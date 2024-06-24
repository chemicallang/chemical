// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include "MembersContainer.h"
#include <optional>
#include <map>
#include "ast/types/ReferencedType.h"
#include "ast/base/ExtendableMembersContainerNode.h"

class StructDefinition : public ExtendableMembersContainerNode {
public:

    InterpretScope *decl_scope;
    std::string name; ///< The name of the struct.
    std::optional<std::unique_ptr<ReferencedType>> overrides;

#ifdef COMPILER_BUILD
    llvm::StructType* llvm_struct_type = nullptr;
#endif

    /**
     * @brief Construct a new StructDeclaration object.
     *
     * @param name The name of the struct.
     * @param fields The members of the struct.
     */
    StructDefinition(
            std::string name,
            const std::optional<std::string>& overrides
    );

    std::string ns_node_identifier() override {
        return name;
    }

    void accept(Visitor *visitor) override;

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    StructDefinition *as_struct_def() override;

    void interpret(InterpretScope &scope) override;

    void interpret_scope_ends(InterpretScope &scope) override;

    ASTNode *child(const std::string &name) override;

    std::unique_ptr<BaseType> create_value_type() override;

    ValueType value_type() const override;

    uint64_t byte_size(bool is64Bit);

    bool requires_destructor();

    FunctionDeclaration* create_destructor();

#ifdef COMPILER_BUILD

    std::vector<llvm::Type *> elements_type(Codegen &gen);

    llvm::Type *llvm_type(Codegen &gen) override;

    void code_gen(Codegen &gen) override;

    llvm::StructType* get_struct_type(Codegen &gen);

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) override;

#endif

};