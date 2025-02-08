// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/base/ExtendableAnnotableNode.h"
#include "ast/base/LocatedIdentifier.h"
#include "ast/types/TypeType.h"

struct TypealiasDeclAttributes {

    /**
     * the access specifier
     */
    AccessSpecifier specifier;

    /**
     * is comptime typealias
     */
    bool is_comptime;

    /**
     * is typealias deprecated
     */
    bool deprecated;

};

class TypealiasStatement : public ExtendableAnnotableNode {
public:

    TypealiasDeclAttributes attrs;
    // before equal
    LocatedIdentifier located_id;
    // after equal
    BaseType* actual_type;
    ASTNode* parent_node;
    SourceLocation location;

    /**
     * @brief Construct a new TypealiasStatement object.
     */
    TypealiasStatement(
            LocatedIdentifier identifier,
            BaseType* actual_type,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : located_id(identifier), actual_type(actual_type), parent_node(parent_node), location(location),
        attrs(specifier, false, false) {

    }

    /**
     * get the name of node
     */
    inline LocatedIdentifier* get_located_id() {
        return &located_id;
    }

    inline bool is_comptime() {
        return attrs.is_comptime;
    }

    inline void set_comptime(bool value) {
        attrs.is_comptime = value;
    }

    inline AccessSpecifier specifier() {
        return attrs.specifier;
    }

    inline void set_specifier(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    inline bool deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    inline const chem::string_view& name_view() const {
        return located_id.identifier;
    }

    inline std::string name_str() const {
        return name_view().str();
    }

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::TypealiasStmt;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    bool is_exported_fast() {
        return specifier() == AccessSpecifier::Public;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    uint64_t byte_size(bool is64Bit) final;

    BaseType* create_value_type(ASTAllocator& allocator) final;

//    hybrid_ptr<BaseType> get_value_type() final;

    BaseType* known_type() final;

    void interpret(InterpretScope &scope);

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) override;

    void accept(Visitor *visitor) final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    void code_gen(Codegen &gen) final;

#endif

};

class ValueTypealiasStmt : public TypealiasStatement {
public:

    Value* value;
    TypeType type_type;

    ValueTypealiasStmt(
        LocatedIdentifier identifier,
        Value* value,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    ) : value(value), TypealiasStatement(identifier, &type_type, parent_node, location, specifier), type_type(location) {

    }

    void interpret(InterpretScope& scope) final;

};