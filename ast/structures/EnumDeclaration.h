// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "EnumMember.h"
#include "ast/base/ExtendableAnnotableNode.h"
#include "ast/base/AccessSpecifier.h"
#include "ast/types/LinkedType.h"
#include "ast/base/LocatedIdentifier.h"

struct EnumDeclAttributes {

    AccessSpecifier specifier;

    bool deprecated;

};

class EnumDeclaration : public ExtendableNode {
private:

    // this is calculated during symbol resolution
    IntNType* underlying_integer_type;

public:

    EnumDeclAttributes attrs;
    LocatedIdentifier located_id; ///< The name of the enum.
    std::unordered_map<chem::string_view, EnumMember*> members; ///< The values of the enum.
    ASTNode* parent_node;
    BaseType* underlying_type;

    /**
     * by default this index starts at zero, however if enum extends another enum
     * we set this to the count of members of that enum + default_starting_index of that index
     * during symbol resolution
     */
    unsigned int default_starting_index = 0;

    // TODO remove this linked_type, we don't want to store stuff, that's not required
    LinkedType linked_type;

    /**
     * @brief Construct a new EnumDeclaration object.
     *
     * @param name The name of the enum.
     * @param values The values of the enum.
     */
    EnumDeclaration(
            LocatedIdentifier name_id,
            BaseType* underlying_type,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ExtendableNode(ASTNodeKind::EnumDecl, location), located_id(name_id), parent_node(parent_node),
        linked_type(name_id.identifier, this, location), underlying_type(underlying_type), attrs(specifier, false) {

    }

    /**
     * get the name of node
     */
    inline LocatedIdentifier* get_located_id() {
        return &located_id;
    }

    inline const chem::string_view& name_view() {
        return located_id.identifier;
    }

    inline bool deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    AccessSpecifier specifier() {
        return attrs.specifier;
    }

    void set_specifier(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    /**
     * this gives the underlying integer type for the enum, it will not return
     * nullptr, because we initialize enum with int type if no type is given by user
     */
    [[nodiscard]]
    inline IntNType* get_underlying_integer_type() const {
        return underlying_integer_type;
    }

    /**
     * this will only return a enum decl if this enum inherits one, otherwise
     * nullptr is returned
     */
    EnumDeclaration* get_inherited_enum_decl();

    /**
     * this would give starting index for the members of this enum
     * please keep in mind, this index doesn't mean the first member will have this value
     * this is the default index given by us if user doesn't explicitly specify the value for the first member
     *
     * this method should only be called after symbol resolution
     */
    [[nodiscard]]
    inline unsigned int get_default_starting_index() const {
        return default_starting_index;
    }


    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) override;

    BaseType* create_value_type(ASTAllocator& allocator) final;

    BaseType* known_type() final;

    uint64_t byte_size(bool is64Bit) final {
        return underlying_type->byte_size(is64Bit);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {
        // do nothing
    }

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

    ASTNode *child(const chem::string_view &name) final;

};