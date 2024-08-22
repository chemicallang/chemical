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
    ASTNode* parent_node;

    /**
     * @brief Construct a new TypealiasStatement object.
     */
    TypealiasStatement(
            std::string identifier,
            std::unique_ptr<BaseType> actual_type,
            ASTNode* parent_node
    );

    ASTNodeKind kind() override {
        return ASTNodeKind::TypealiasStmt;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    TypealiasStatement* as_typealias() override {
        return this;
    }

    std::string ns_node_identifier() override {
        return identifier;
    }

    uint64_t byte_size(bool is64Bit) override;

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    BaseType* known_type() override;

    void interpret(InterpretScope &scope) override;

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    void accept(Visitor *visitor) override;

    [[nodiscard]]
    BaseTypeKind type_kind() const override;

    [[nodiscard]]
    ValueType value_type() const override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    void code_gen(Codegen &gen) override;

#endif

};