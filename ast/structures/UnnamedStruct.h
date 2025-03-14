// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "VariablesContainer.h"
#include "BaseDefMember.h"
#include "ast/types/StructType.h"

class UnnamedStruct : public BaseDefMember, public VariablesContainer {
public:

    AccessSpecifier specifier;

    /**
     * constructor
     */
    UnnamedStruct(
        chem::string_view name,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    ) : BaseDefMember(name, ASTNodeKind::UnnamedStruct, parent_node, location), specifier(specifier) {

    }


//    BaseType* known_type() final {
//        return this;
//    }

//    std::string get_runtime_name() final {
//        return "";
//    }

//    VariablesContainer *variables_container() final {
//        return this;
//    }

    VariablesContainer *as_variables_container() final {
        return this;
    }

//    int16_t get_generic_iteration() final {
//        return 0;
//    }

    bool get_is_const() final {
        // TODO allow user to mark unnamed structs const
        return false;
    }

    UnnamedStruct* copy_member(ASTAllocator &allocator) final {
        const auto unnamed = new (allocator.allocate<UnnamedStruct>()) UnnamedStruct(name, parent(), encoded_location());
        VariablesContainer::copy_into(*unnamed, allocator, this);
        return unnamed;
    }

    void redeclare_top_level(SymbolResolver &linker) final;

    void link_signature(SymbolResolver &linker) override;

    bool requires_copy_fn() {
        for(const auto var : variables()) {
            if(var->known_type()->requires_copy_fn()) {
                return true;
            }
        }
        return false;
    }

    bool requires_destructor() {
        for(const auto var : variables()) {
            if(var->known_type()->requires_destructor()) {
                return true;
            }
        }
        return false;
    }

    ASTNode *child(const chem::string_view &name) final {
        return VariablesContainer::child_def_member(name);
    }

    uint64_t byte_size(bool is64Bit) final {
        return total_byte_size(is64Bit);
    }

//    ASTNode *linked_node() final {
//        return this;
//    }

    BaseType* create_value_type(ASTAllocator &allocator) final;

//    [[nodiscard]]
//    BaseType* copy(ASTAllocator& allocator) const final;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

    bool add_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const chem::string_view &name
    ) final {
        return VariablesContainer::llvm_struct_child_index(gen, indexes, name);
    }

#endif

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return BaseTypeKind::Struct;
    }

};