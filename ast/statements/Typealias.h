// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/base/ExtendableAnnotableNode.h"

class TypealiasStatement : public ExtendableAnnotableNode {
public:

    AccessSpecifier specifier;
    // before equal
    std::string identifier;
    // after equal
    BaseType* actual_type;
    ASTNode* parent_node;
    CSTToken* token;

    /**
     * @brief Construct a new TypealiasStatement object.
     */
    TypealiasStatement(
            std::string identifier,
            BaseType* actual_type,
            ASTNode* parent_node,
            CSTToken* token,
            AccessSpecifier specifier = AccessSpecifier::Internal
    );

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::TypealiasStmt;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    bool is_exported_fast() {
        return specifier == AccessSpecifier::Public;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    const std::string& ns_node_identifier() override {
        return identifier;
    }

    uint64_t byte_size(bool is64Bit) override;

    BaseType* create_value_type(ASTAllocator& allocator) override;

//    hybrid_ptr<BaseType> get_value_type() override;

    BaseType* known_type() override;

    void interpret(InterpretScope &scope) override;

    void declare_top_level(SymbolResolver &linker) override;

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