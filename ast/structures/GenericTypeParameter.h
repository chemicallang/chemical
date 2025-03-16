// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

class GenericTypeParameter : public ASTNode {
private:

    /**
     * we set this to an active type, before instantiating the generic decl
     */
    BaseType* active_type = nullptr;

public:

    chem::string_view identifier;
    BaseType* at_least_type;
    BaseType* def_type;
    std::vector<BaseType*> usage;
    // TODO use int32 here
     // <-- index of active type in usage vector
    unsigned param_index = 0; // <-- index in the generic type parameters

    /**
     * constructor
     */
    constexpr GenericTypeParameter(
        chem::string_view identifier,
        BaseType* at_least_type,
        BaseType* def_type,
        ASTNode* parent_node,
        unsigned param_index,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::GenericTypeParam, parent_node, location), identifier(identifier),
        at_least_type(at_least_type), def_type(def_type), param_index(param_index)
    {

    }

    void declare_only(SymbolResolver& linker);

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void register_usage(ASTAllocator& allocator, BaseType* type);

    void set_active_iteration(int iteration) {
#ifdef DEBUG
        if(iteration < 0 || iteration >= usage.size()) {
            throw std::runtime_error("wrong iteration");
        }
#endif
        active_type = usage[iteration];
    }

    void deactivate_iteration() {
        active_type = nullptr;
    }

    BaseType *known_type() final {
        return active_type;
    }

    /**
     * this method is called by the generic instantiator
     */
    inline BaseType* concrete_type() {
        return active_type;
    }

    inline ASTNode* active_linked() {
        return active_type ? active_type->linked_node() : nullptr;
    }

    ASTNode *child(const chem::string_view &name) final {
        const auto linked = active_linked();
        return linked ? linked->child(name) : (at_least_type ? at_least_type->linked_node()->child(name) : nullptr);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_param_type(Codegen &gen) final {
        return active_type->llvm_param_type(gen);
    }

    llvm::Type *llvm_type(Codegen &gen) final {
        return active_type->llvm_type(gen);
    }

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final {
        return active_type->llvm_chain_type(gen, values, index);
    }

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final {
        return active_type->linked_node()->add_child_index(gen, indexes, name);
    }

#endif


};