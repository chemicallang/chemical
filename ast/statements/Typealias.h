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

    CSTToken *cst_token() final {
        return token;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::TypealiasStmt;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    bool is_exported_fast() {
        return specifier == AccessSpecifier::Public;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    const std::string& ns_node_identifier() final {
        return identifier;
    }

    uint64_t byte_size(bool is64Bit) final;

    BaseType* create_value_type(ASTAllocator& allocator) final;

//    hybrid_ptr<BaseType> get_value_type() final;

    BaseType* known_type() final;

    void interpret(InterpretScope &scope) final;

    void declare_top_level(SymbolResolver &linker) final;

    void accept(Visitor *visitor) final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

    [[nodiscard]]
    ValueType value_type() const final;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    void code_gen(Codegen &gen) final;

#endif

};